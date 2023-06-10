/*
 *   Copyright (c) 2023 Anton Kundenko <singaraiona@gmail.com>
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.

 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "hash.h"
#include "rayforce.h"
#include "alloc.h"
#include "util.h"

ht_t *ht_new(i64_t size, u64_t (*hasher)(i64_t a), i32_t (*compare)(i64_t a, i64_t b))
{
    size = next_power_of_two_u64(size);
    i64_t i, *kv;
    ht_t *table = (ht_t *)rf_malloc(sizeof(struct ht_t));

    table->keys = vector_i64(size);
    table->vals = vector_i64(size);
    table->size = size;
    table->count = 0;
    table->hasher = hasher;
    table->compare = compare;

    kv = as_vector_i64(&table->keys);

    for (i = 0; i < size; i++)
        kv[i] = NULL_I64;

    return table;
}

null_t ht_free(ht_t *table)
{
    rf_object_free(&table->keys);
    rf_object_free(&table->vals);
    rf_free(table);
}

null_t ht_rehash(ht_t *table)
{
    i64_t i, old_size = table->size;
    rf_object_t old_keys = table->keys;
    rf_object_t old_vals = table->vals;
    i64_t *ok = as_vector_i64(&old_keys), *ov = as_vector_i64(&old_vals), *nk, *nv, key, val, factor, index;

    // Double the table size.
    table->size *= 2;
    debug("rehashing to %lld\n", table->size);
    table->keys = vector_i64(table->size);
    table->vals = vector_i64(table->size);
    factor = table->size - 1;

    nk = as_vector_i64(&table->keys);
    nv = as_vector_i64(&table->vals);

    for (i = 0; i < table->size; i++)
        nk[i] = NULL_I64;

    for (i = 0; i < old_size; i++)
    {
        if (ok[i] != NULL_I64)
        {
            key = ok[i];
            val = ov[i];
            index = table->hasher(key) & factor;

            // Linear probing.
            while (nk[index] != NULL_I64)
            {
                if (index == table->size)
                    panic("ht is full!!");
                index = index + 1;
            }

            nk[index] = key;
            nv[index] = val;
        }
    }

    rf_object_free(&old_keys);
    rf_object_free(&old_vals);
}

/*
 * Inserts new node or returns existing node.
 * Does not update existing node.
 */
i64_t ht_insert(ht_t *table, i64_t key, i64_t val)
{
    while (true)
    {
        i32_t i = 0, size = table->size;
        i64_t factor = table->size - 1,
              index = table->hasher(key) & factor;

        i64_t *keys = as_vector_i64(&table->keys);
        i64_t *vals = as_vector_i64(&table->vals);

        for (i = index; i < size; i++)
        {
            if (keys[i] != NULL_I64)
            {
                if (table->compare(keys[i], key) == 0)
                    return vals[i];
            }
            else
            {
                keys[i] = key;
                vals[i] = val;
                table->count++;

                // Check if ht_rehash is necessary.
                if ((f64_t)table->count / table->size > 0.7)
                    ht_rehash(table);

                return val;
            }
        }

        ht_rehash(table);
    }
}

/*
 * Does the same as ht_insert, but uses a function to set the rf_object of the bucket.
 */
i64_t ht_insert_with(ht_t *table, i64_t key, i64_t val, null_t *seed,
                     i64_t (*func)(i64_t key, i64_t val, null_t *seed, i64_t *tkey, i64_t *tval))
{
    while (true)
    {
        i32_t i, size = table->size;
        u64_t factor = table->size - 1,
              index = table->hasher(key) & factor;

        i64_t *keys = as_vector_i64(&table->keys);
        i64_t *vals = as_vector_i64(&table->vals);

        for (i = index; i < size; i++)
        {
            if (keys[i] != NULL_I64)
            {
                if (table->compare(keys[i], key) == 0)
                    return vals[i];
            }
            else
            {
                table->count++;

                // Check if ht_rehash is necessary.
                if ((f64_t)table->count / table->size > 0.7)
                    ht_rehash(table);

                return func(key, val, seed, &keys[i], &vals[i]);
            }
        }

        ht_rehash(table);
    }
}

/*
 * Inserts new node or updates existing one.
 * Returns true if the node was updated, false if it was inserted.
 */
bool_t ht_upsert(ht_t *table, i64_t key, i64_t val)
{
    while (true)
    {
        i32_t i, size = table->size;
        u64_t factor = table->size - 1,
              index = table->hasher(key) & factor;

        i64_t *keys = as_vector_i64(&table->keys);
        i64_t *vals = as_vector_i64(&table->vals);

        for (i = index; i < size; i++)
        {
            if (keys[i] != NULL_I64)
            {
                if (table->compare(keys[i], key) == 0)
                {
                    vals[i] = val;
                    return true;
                }
            }
            else
            {
                keys[i] = key;
                vals[i] = val;
                table->count++;

                // Check if ht_rehash is necessary.
                if ((f64_t)table->count / table->size > 0.7)
                    ht_rehash(table);

                return false;
            }
        }

        ht_rehash(table);
    }
}

/*
 * Does the same as ht_upsert, but uses a function to set the val of the bucket.
 */
bool_t ht_upsert_with(ht_t *table, i64_t key, i64_t val, null_t *seed,
                      i64_t (*func)(i64_t key, i64_t val, null_t *seed, i64_t *tkey, i64_t *tval))
{
    while (true)
    {
        i32_t i, size = table->size;
        u64_t factor = table->size - 1,
              index = table->hasher(key) & factor;

        i64_t *keys = as_vector_i64(&table->keys);
        i64_t *vals = as_vector_i64(&table->vals);

        for (i = index; i < size; i++)
        {
            if (keys[i] != NULL_I64)
            {
                if (table->compare(keys[i], key) == 0)
                {
                    func(key, val, seed, &keys[i], &vals[i]);
                    return true;
                }
            }
            else
            {
                keys[i] = key;
                vals[i] = val;
                table->count++;

                // Check if ht_rehash is necessary.
                if ((f64_t)table->count / table->size > 0.7)
                    ht_rehash(table);

                return false;
            }
        }

        ht_rehash(table);
    }
}

bool_t ht_upsert_with2(ht_t *table, i64_t key, i64_t val, null_t *seed,
                       i64_t (*ifunc)(i64_t key, i64_t val, null_t *seed, i64_t *tkey, i64_t *tval),
                       i64_t (*ufunc)(i64_t key, i64_t val, null_t *seed, i64_t *tkey, i64_t *tval))
{
    while (true)
    {
        i32_t i, size = table->size;
        u64_t factor = table->size - 1,
              index = table->hasher(key) & factor;

        i64_t *keys = as_vector_i64(&table->keys);
        i64_t *vals = as_vector_i64(&table->vals);

        // Check if ht_rehash is necessary.
        if ((f64_t)table->count / table->size > 0.7)
            ht_rehash(table);

        for (i = index; i < size; i++)
        {
            if (keys[i] == NULL_I64)
            {
                ifunc(key, val, seed, &keys[i], &vals[i]);
                table->count++;
                return false;
            }

            if (table->compare(keys[i], key) == 0)
            {
                ufunc(key, val, seed, &keys[i], &vals[i]);
                return true;
            }
        }

        ht_rehash(table);
    }
}

/*
 * Returns the rf_object of the node with the given key.
 * Returns -1 if the key does not exist.
 */
i64_t ht_get(ht_t *table, i64_t key)
{
    i32_t i, size = table->size;
    u64_t factor = table->size - 1,
          index = table->hasher(key) & factor;
    i64_t *keys = as_vector_i64(&table->keys);
    i64_t *vals = as_vector_i64(&table->vals);

    for (i = index; i < size; i++)
    {
        if (keys[i] != NULL_I64)
        {
            if (table->compare(keys[i], key) == 0)
                return vals[i];
        }
        else
            return NULL_I64;
    }

    return NULL_I64;
}

bool_t ht_next_entry(ht_t *table, i64_t **k, i64_t **v, i64_t *index)
{
    i64_t i, *keys = as_vector_i64(&table->keys),
             *vals = as_vector_i64(&table->vals);

    while (*index < table->size)
    {
        if (keys[*index] != NULL_I64)
        {
            i = *index;
            (*index)++;
            *k = &keys[i];
            *v = &vals[i];
            return true;
        }

        (*index)++;
    }

    return false;
}
