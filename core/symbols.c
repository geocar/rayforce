#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "symbols.h"
#include "storm.h"
#include "alloc.h"

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

/*
 * djb2
 * by Dan Bernstein
 * http://www.cse.yorku.ca/~oz/hash.html
 */
u64_t hash(str_t str)
{
    u64_t hash = 5381;
    u8_t c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

symbols_t *symbols_create()
{
    symbols_t *table = (symbols_t *)storm_malloc(sizeof(symbols_t));
    memset(table, 0, sizeof(symbols_t));

    return table;
}

null_t symbols_free(symbols_t *symbols)
{
    for (u64_t i = 0; i < CELLS_SIZE; i++)
    {
        cell_t *cell = &symbols->cells[i];
        cell_t *next;

        while (cell)
        {
            next = cell->next;
            storm_free(cell->str);
            storm_free(cell);
            cell = next;
        }
    }

    storm_free(symbols);
}

str_t strdup(str_t str)
{
    u64_t len = strlen(str) + 1;
    str_t new_str = (str_t)storm_malloc(len);
    strncpy(new_str, str, len);
    return new_str;
}

i64_t symbols_insert(symbols_t *symbols, str_t str)
{
    str_t new_str;
    u64_t key = hash(str) % CELLS_SIZE;
    cell_t *cell, *next;
    next = &symbols->cells[key];

    // With good hash function, this is most likely to happen
    if (likely(!next->str))
    {
        // Add new cell to the list
        next->str = strdup(str);
        next->key = symbols->size;
        symbols->keys[symbols->size++] = next;
        return next->key;
    }

    // Otherwise we have to iterate through the list of collisions
    do
    {
        cell = next;

        // Key already exists, return it
        if (strcmp(cell->str, str) == 0)
            return cell->key;

    } while (next = cell->next);

    // Add new sell to the end of the list
    next = (cell_t *)storm_malloc(sizeof(cell_t));
    next->str = strdup(str);
    next->next = NULL;
    next->key = symbols->size;
    cell->next = next;
    symbols->keys[symbols->size++] = next;

    return next->key;
}

str_t symbols_get(symbols_t *symbols, i64_t key)
{
    cell_t *cell = symbols->keys[key];
    return cell->str;
}