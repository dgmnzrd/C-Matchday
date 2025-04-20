// tests/test_api.c

#include "football_api.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void) {
    // 1) Probar fetch_competitions()
    char *cats = fetch_competitions();
    assert(cats != NULL && strlen(cats) > 0);
    printf("✅ fetch_competitions devolvió %zu bytes\n", strlen(cats));
    free(cats);

    // 2) Probar fetch_matchday_results() para una fecha cualquiera
    //    Si no hay partidos, al menos la función no debe crashar
    const char *dateFrom = "2025-04-01";
    const char *dateTo   = "2025-04-01";
    char *res = fetch_matchday_results(dateFrom, dateTo, "");
    if (res) {
        printf("⚽ fetch_matchday_results(%s–%s) devolvió %zu bytes\n",
               dateFrom, dateTo, strlen(res));
        free(res);
    } else {
        printf("⚠️ fetch_matchday_results(%s–%s) devolvió NULL (pero no crash)\n",
               dateFrom, dateTo);
    }

    return 0;
}