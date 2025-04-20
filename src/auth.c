// src/auth.c

#include "auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"
#include <stdbool.h>

#define CONFIG_PATH "config/firebase_config.json"  // Ajustado para cwd = project root
#define MAX_URL 256

// Buffer para recibir respuesta HTTP
typedef struct {
    char *ptr;
    size_t len;
} Buffer;

static size_t write_cb(void *data, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    Buffer *buf = (Buffer *)userp;
    char *tmp = realloc(buf->ptr, buf->len + total + 1);
    if (!tmp) return 0;
    buf->ptr = tmp;
    memcpy(buf->ptr + buf->len, data, total);
    buf->len += total;
    buf->ptr[buf->len] = '\0';
    return total;
}

// Lee el apiKey de config/firebase_config.json
static char *load_api_key(void) {
    printf("Leyendo config desde: %s\n", CONFIG_PATH);
    FILE *f = fopen(CONFIG_PATH, "rb");
    if (!f) {
        fprintf(stderr, "ERROR: no pude abrir %s\n", CONFIG_PATH);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *txt = malloc(sz + 1);
    if (!txt) {
        fclose(f);
        return NULL;
    }
    fread(txt, 1, sz, f);
    fclose(f);
    txt[sz] = '\0';

    cJSON *cfg = cJSON_Parse(txt);
    free(txt);
    if (!cfg) {
        fprintf(stderr, "ERROR: JSON inválido en %s\n", CONFIG_PATH);
        return NULL;
    }

    cJSON *item = cJSON_GetObjectItem(cfg, "apiKey");
    char *apiKey = (item && cJSON_IsString(item))
                   ? strdup(item->valuestring)
                   : NULL;
    cJSON_Delete(cfg);
    if (!apiKey) {
        fprintf(stderr, "ERROR: no encontré 'apiKey' en %s\n", CONFIG_PATH);
    }
    return apiKey;
}

// Función genérica para SIGNUP y SIGNIN
static bool call_firebase(const char *email, const char *pw,
                          const char *path, char **out_token) {
    char *apiKey = load_api_key();
    if (!apiKey) return false;

    char url[MAX_URL];
    snprintf(url, sizeof(url),
             "https://identitytoolkit.googleapis.com/v1/accounts:%s?key=%s",
             path, apiKey);
    free(apiKey);

    // Preparamos JSON de body
    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "email", email);
    cJSON_AddStringToObject(body, "password", pw);
    cJSON_AddBoolToObject(body, "returnSecureToken", true);
    char *body_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);

    // Debug: muestra URL y body
    printf("Llamando a Firebase:\n  URL: %s\n  Body: %s\n", url, body_str);

    // Buffer de respuesta
    Buffer buf = { .ptr = malloc(1), .len = 0 };

    // Configuramos Curl
    CURL *curl = curl_easy_init();
    if (!curl) {
        free(body_str);
        free(buf.ptr);
        return false;
    }

    struct curl_slist *hdrs = NULL;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);

    CURLcode res = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
    free(body_str);

    if (res != CURLE_OK || code != 200) {
        fprintf(stderr, "ERROR: HTTP %ld en Firebase (%s)\n", code, url);
        free(buf.ptr);
        return false;
    }

    // Parsear idToken
    cJSON *resp = cJSON_Parse(buf.ptr);
    free(buf.ptr);
    if (!resp) {
        fprintf(stderr, "ERROR: JSON inválido en respuesta de Firebase\n");
        return false;
    }
    cJSON *token = cJSON_GetObjectItem(resp, "idToken");
    if (!token || !cJSON_IsString(token)) {
        fprintf(stderr, "ERROR: 'idToken' no encontrado en respuesta\n");
        cJSON_Delete(resp);
        return false;
    }
    *out_token = strdup(token->valuestring);
    cJSON_Delete(resp);
    return true;
}

bool firebase_register(const char *email, const char *password, char **id_token) {
    return call_firebase(email, password, "signUp", id_token);
}

bool firebase_login(const char *email, const char *password, char **id_token) {
    return call_firebase(email, password, "signInWithPassword", id_token);
}