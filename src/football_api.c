// src/football_api.c

#include "football_api.h"
#include <curl/curl.h>
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_URL        512
#define BESOCER_URL    "http://apiclient.besoccerapps.com/scripts/api/api.php"
#define BESO_TZ        "America/Monterrey"

// Buffer para la respuesta de cURL
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

// Trata de abrir el config.json en dos rutas posibles
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

// Lee footballApiToken desde el config JSON
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
    if (tok) {
        fprintf(stderr, "[DEBUG] Token cargado: %.5s…\n", tok);
    } else {
        fprintf(stderr, "[ERROR] 'footballApiToken' no encontrado en config\n");
    }
    return tok;
}

// Codifica ':' → "%3A" y ',' → "%2C" para el parámetro filter
static char *encode_filter(const char *filter) {
    size_t len = strlen(filter);
    // cada ':' o ',' ocupa 3 caracteres en la versión codificada
    char *out = malloc(len * 3 + 1);
    char *p = out;
    for (size_t i = 0; i < len; i++) {
        switch (filter[i]) {
            case ':':
                memcpy(p, "%3A", 3);
                p += 3;
                break;
            case ',':
                memcpy(p, "%2C", 3);
                p += 3;
                break;
            default:
                *p++ = filter[i];
                break;
        }
    }
    *p = '\0';
    return out;
}

// Función interna para GET a BeSoccer nivel 1
static char *fetch_besoccer(const char *req, const char *filter) {
    char *key = load_token();
    if (!key) return NULL;

    char url[MAX_URL];
    if (filter && *filter) {
        // codificamos filter antes de montarlo en la URL
        char *ef = encode_filter(filter);
        snprintf(url, sizeof(url),
                 "%s?key=%s&tz=%s&req=%s&filter=%s&format=json",
                 BESOCER_URL, key, BESO_TZ, req, ef);
        free(ef);
    } else {
        snprintf(url, sizeof(url),
                 "%s?key=%s&tz=%s&req=%s&format=json",
                 BESOCER_URL, key, BESO_TZ, req);
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

// API pública: devuelve JSON con las ligas
char *fetch_competitions(void) {
    return fetch_besoccer("categories", "my_leagues");
}

// API pública: devuelve JSON con los partidos de dateFrom–dateTo,
//  y opcionalmente filtrados por competencia
char *fetch_matchday_results(const char *dateFrom,
                             const char *dateTo,
                             const char *compId) {
    char filter[128];
    if (compId && *compId) {
        snprintf(filter, sizeof(filter),
                 "competitions:%s,dateFrom:%s,dateTo:%s",
                 compId, dateFrom, dateTo);
    } else {
        snprintf(filter, sizeof(filter),
                 "dateFrom:%s,dateTo:%s",
                 dateFrom, dateTo);
    }
    return fetch_besoccer("fixtures", filter);
}