#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>

bool firebase_login(const char *email, const char *password, char **id_token);
bool firebase_register(const char *email, const char *password, char **id_token);

#endif // AUTH_H