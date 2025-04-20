#include <assert.h>
#include <stdlib.h>
#include "auth.h"

int main(void) {
    char *token = NULL;
    bool ok = firebase_login("test@example.com", "password", &token);
    assert(ok == false);
    return 0;
}