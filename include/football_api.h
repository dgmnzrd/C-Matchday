// src/football_api.h

#ifndef FOOTBALL_API_H
#define FOOTBALL_API_H

// Devuelve un string con el JSON de categorías (ligas). Hay que free() la cadena.
char *fetch_competitions(void);

// Devuelve un string con el JSON de partidos del rango dado y opcional competencia.
// Pasar compId="" o NULL para no filtrar por liga. Hay que free() la cadena.
char *fetch_matchday_results(const char *dateFrom,
                             const char *dateTo,
                             const char *compId);

#endif // FOOTBALL_API_H