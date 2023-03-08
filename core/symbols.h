#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "storm.h"

#define CELLS_SIZE 4096

typedef struct cell_t
{
    i64_t key;
    str_t str;
    struct cell_t *next;
} cell_t;

typedef struct symbols_t
{
    i64_t size;
    cell_t cells[CELLS_SIZE];
    cell_t *keys[CELLS_SIZE];
} symbols_t;

i64_t symbols_insert(symbols_t *symbols, str_t str);
str_t symbols_get(symbols_t *symbols, i64_t key);

symbols_t *symbols_create();
null_t symbols_free(symbols_t *symbols);

#endif
