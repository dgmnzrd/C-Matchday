#include "auth.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

// TODO: Implementar llamadas REST a Firebase Auth
bool firebase_login(const char *email, const char *password, char **id_token) {
    (void)email; (void)password; (void)id_token;
    return false;
}

bool firebase_register(const char *email, const char *password, char **id_token) {
    (void)email; (void)password; (void)id_token;
    return false;
}