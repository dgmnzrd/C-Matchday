#include <assert.h>
#include <stddef.h>
#include "football_api.h"

int main(void) {
    char *res = fetch_matchday_results("https://api.football-data.org/v2/matches", "dateFrom=2025-01-01");
    assert(res == NULL);
    return 0;
}