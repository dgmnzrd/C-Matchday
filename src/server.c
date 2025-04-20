// src/server.c

#include "mongoose.h"
#include "auth.h"
#include "football_api.h"   // para fetch_matchday_results()
#include "cJSON.h"
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define WEB_ROOT      "public"
#define BESO_URL_BASE "http://apiclient.besoccerapps.com/scripts/api/api.php"
#define BESO_TZ       "America/Monterrey"

// --- Buffer para libcurl ---
typedef struct {
    char *ptr;
    size_t len;
} Buffer;

static size_t write_cb(void *data, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    Buffer *buf = (Buffer *)userp;
    char *tmp   = realloc(buf->ptr, buf->len + total + 1);
    if (!tmp) return 0;
    buf->ptr = tmp;
    memcpy(buf->ptr + buf->len, data, total);
    buf->len += total;
    buf->ptr[buf->len] = '\0';
    return total;
}

// Intenta abrir el config en dos rutas
static FILE *open_config(void) {
    const char *paths[] = {
        "config/football_config.json",
        "../config/football_config.json"
    };
    for (int i = 0; i < 2; i++) {
        FILE *f = fopen(paths[i], "rb");
        if (f) {
            fprintf(stderr, "[DEBUG] Abriendo config en %s\n", paths[i]);
            return f;
        }
    }
    return NULL;
}

// Carga la API‑Key desde el JSON de config
static char *load_token(void) {
    FILE *f = open_config();
    if (!f) {
        fprintf(stderr, "[ERROR] No pude abrir ningún config/football_config.json\n");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *txt = malloc(sz + 1);
    if (!txt) { fclose(f); return NULL; }
    fread(txt, 1, sz, f);
    fclose(f);
    txt[sz] = '\0';

    cJSON *cfg = cJSON_Parse(txt);
    free(txt);
    if (!cfg) {
        fprintf(stderr, "[ERROR] JSON inválido en config\n");
        return NULL;
    }
    cJSON *item = cJSON_GetObjectItem(cfg, "footballApiToken");
    char *tok = (item && cJSON_IsString(item)) ? strdup(item->valuestring) : NULL;
    cJSON_Delete(cfg);
    if (!tok) {
        fprintf(stderr, "[ERROR] 'footballApiToken' no encontrado en config\n");
    } else {
        fprintf(stderr, "[DEBUG] Token cargado: %.5s…\n", tok);
    }
    return tok;
}

// Hace GET a BeSoccer nivel 1 (categories o fixtures)
static char *fetch_besoccer(const char *req, const char *filter) {
    char *key = load_token();
    if (!key) return NULL;

    char url[512];
    if (filter && filter[0]) {
        // Único parámetro filter: condiciones separadas por comas, usando ':'
        snprintf(url, sizeof(url),
                 "%s?key=%s&tz=%s&req=%s&filter=%s&format=json",
                 BESO_URL_BASE, key, BESO_TZ, req, filter);
    } else {
        snprintf(url, sizeof(url),
                 "%s?key=%s&tz=%s&req=%s&format=json",
                 BESO_URL_BASE, key, BESO_TZ, req);
    }
    free(key);
    fprintf(stderr, "[DEBUG] GET %s\n", url);

    Buffer buf = { malloc(1), 0 };
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL,           url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &buf);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || code < 200 || code >= 300) {
        fprintf(stderr, "[ERROR] HTTP %ld al pedir %s\n", code, url);
        free(buf.ptr);
        return NULL;
    }
    return buf.ptr;
}

static void on_http_event(struct mg_connection *c, int ev, void *ev_data) {
    if (ev != MG_EV_HTTP_MSG) return;
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    struct mg_http_serve_opts opts = { .root_dir = WEB_ROOT };

    // ---- POST /login ----
    if (hm->method.len == 4 &&
        !strncmp(hm->method.buf, "POST", 4) &&
        hm->uri.len    == 6 &&
        !strncmp(hm->uri.buf, "/login", 6)) {

        cJSON *js = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
        const cJSON *email = cJSON_GetObjectItem(js, "email");
        const cJSON *pw    = cJSON_GetObjectItem(js, "password");
        char *token = NULL;
        if (cJSON_IsString(email) && cJSON_IsString(pw) &&
            firebase_login(email->valuestring, pw->valuestring, &token)) {
            mg_http_reply(c, 200,
                          "Content-Type: application/json\r\n",
                          "{\"idToken\":\"%s\"}", token);
            free(token);
        } else {
            mg_http_reply(c, 401,
                          "Content-Type: text/plain\r\n",
                          "Login failed");
        }
        cJSON_Delete(js);
        return;
    }

    // ---- POST /register ----
    if (hm->method.len == 4 &&
        !strncmp(hm->method.buf, "POST", 4) &&
        hm->uri.len    == 9 &&
        !strncmp(hm->uri.buf, "/register", 9)) {

        cJSON *js = cJSON_ParseWithLength(hm->body.buf, hm->body.len);
        const cJSON *email = cJSON_GetObjectItem(js, "email");
        const cJSON *pw    = cJSON_GetObjectItem(js, "password");
        char *token = NULL;
        if (cJSON_IsString(email) && cJSON_IsString(pw) &&
            firebase_register(email->valuestring, pw->valuestring, &token)) {
            mg_http_reply(c, 200,
                          "Content-Type: application/json\r\n",
                          "{\"idToken\":\"%s\"}", token);
            free(token);
        } else {
            mg_http_reply(c, 400,
                          "Content-Type: text/plain\r\n",
                          "Registration failed");
        }
        cJSON_Delete(js);
        return;
    }

    // ---- GET /competitions ----
    if (hm->method.len == 3 &&
        !strncmp(hm->method.buf, "GET", 3) &&
        hm->uri.len    == 13 &&
        !strncmp(hm->uri.buf, "/competitions", 13)) {

        char *json = fetch_besoccer("categories", "my_leagues");
        if (json) {
            mg_http_reply(c, 200,
                          "Content-Type: application/json\r\n",
                          "%s", json);
            free(json);
        } else {
            mg_http_reply(c, 500,
                          "Content-Type: text/plain\r\n",
                          "Error fetching competitions");
        }
        return;
    }

    // ---- GET / (fixtures si hay query) ----
    if (hm->method.len == 3 &&
        !strncmp(hm->method.buf, "GET", 3) &&
        hm->uri.len    == 1 &&
        hm->uri.buf[0] == '/') {

        // extraer dateFrom, dateTo, competitions
        char dateFrom[16] = "", dateTo[16] = "", compId[16] = "";
        mg_http_get_var(&hm->query, "dateFrom",     dateFrom, sizeof(dateFrom));
        mg_http_get_var(&hm->query, "dateTo",       dateTo,   sizeof(dateTo));
        mg_http_get_var(&hm->query, "competitions", compId,   sizeof(compId));

        if (dateFrom[0] || dateTo[0] || compId[0]) {
            // fechas por defecto = hoy
            if (!dateFrom[0] || !dateTo[0]) {
                time_t t = time(NULL);
                struct tm *tm = localtime(&t);
                strftime(dateFrom, sizeof(dateFrom), "%Y-%m-%d", tm);
                strcpy(dateTo, dateFrom);
            }
            // filtro único con ':'
            char filter[80];
            if (compId[0]) {
                snprintf(filter, sizeof(filter),
                         "competitions:%s,dateFrom:%s,dateTo:%s",
                         compId, dateFrom, dateTo);
            } else {
                snprintf(filter, sizeof(filter),
                         "dateFrom:%s,dateTo:%s",
                         dateFrom, dateTo);
            }
            char *json = fetch_besoccer("fixtures", filter);
            if (json) {
                mg_http_reply(c, 200,
                              "Content-Type: application/json\r\n",
                              "%s", json);
                free(json);
            } else {
                mg_http_reply(c, 500,
                              "Content-Type: text/plain\r\n",
                              "Error fetching matches");
            }
            return;
        }

        // sin query: sirvo el index.html
        mg_http_serve_file(c, hm, WEB_ROOT "/index.html", &opts);
        return;
    }

    // ---- GET /login, /register ----
    if (hm->method.len == 3 && !strncmp(hm->method.buf, "GET", 3)) {
        if (hm->uri.len == 6 && !strncmp(hm->uri.buf, "/login", 6)) {
            mg_http_serve_file(c, hm, WEB_ROOT "/login.html", &opts);
            return;
        }
        if (hm->uri.len == 9 && !strncmp(hm->uri.buf, "/register", 9)) {
            mg_http_serve_file(c, hm, WEB_ROOT "/register.html", &opts);
            return;
        }
    }

    // ---- Restantes estáticos (css, js, imágenes) ----
    mg_http_serve_dir(c, hm, &opts);
}

void initialize_server(const char *host, int port) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    char addr[64];
    snprintf(addr, sizeof(addr), "http://%s:%d", host, port);
    if (!mg_http_listen(&mgr, addr, on_http_event, NULL)) {
        fprintf(stderr, "ERROR: no pude escuchar en %s\n", addr);
        return;
    }
    printf("Servidor escuchando en %s, sirviendo %s/\n", addr, WEB_ROOT);

    for (;;) mg_mgr_poll(&mgr, 1000);
    mg_mgr_free(&mgr);
}