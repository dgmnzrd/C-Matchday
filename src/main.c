#include "server.h"
#include <stdio.h>

int main(void) {
    const char *host = "0.0.0.0";
    int port = 8080;
    printf("Starting C-Matchday server on %s:%d...\n", host, port);
    initialize_server(host, port);
    return 0;
}