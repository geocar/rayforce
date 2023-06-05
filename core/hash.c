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

null_t rehash(ht_t *table)
{
    i64_t i, old_size = table->size;
    rf_object_t old_keys = table->keys;
    rf_object_t old_vals = table->vals;
    i64_t *ok = as_vector_i64(&old_keys), *ov = as_vector_i64(&old_vals), *kv;

    // Double the table size.
    table->size *= 2;
    table->keys = vector_i64(table->size);
    table->vals = vector_i64(table->size);

    for (i = 0; i < old_size; i++)
    {
        if (as_vector_i64(&old_keys)[i] != NULL_I64)
            ht_insert(table, ok[i], ov[i]);
    }

    kv = as_vector_i64(&table->keys);

    for (; i < table->size; i++)
        kv[i] = NULL_I64;

    rf_object_free(&old_keys);
    rf_object_free(&old_vals);
}

null_t rehash_with(ht_t *table, null_t *seed, i64_t (*func)(i64_t key, i64_t val, null_t *seed, i64_t *tkey, i64_t *tval))
{
    i64_t i, old_size = table->size;
    rf_object_t old_keys = table->keys;
    rf_object_t old_vals = table->vals;
    i64_t *ok = as_vector_i64(&old_keys), *ov = as_vector_i64(&old_vals), *kv;

    // Double the table size.
    table->size *= 2;
    table->keys = vector_i64(table->size);
    table->vals = vector_i64(table->size);

    for (i = 0; i < old_size; i++)
    {
        if (as_vector_i64(&old_keys)[i] != NULL_I64)
            ht_insert_with(table, ok[i], ov[i], seed, func);
    }

    kv = as_vector_i64(&table->keys);

    for (; i < table->size; i++)
        kv[i] = NULL_I64;

    rf_object_free(&old_keys);
    rf_object_free(&old_vals);
}

/*
 * Inserts new node or returns existing node.
 * Does not update existing node.
 */
i64_t ht_insert(ht_t *table, i64_t key, i64_t val)
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

            continue;
        }

        keys[i] = key;
        vals[i] = val;
        table->count++;
        return val;
    }

    rehash(table);
    return ht_insert(table, key, val);
}

/*
 * Does the same as ht_insert, but uses a function to set the rf_object of the bucket.
 */
i64_t ht_insert_with(ht_t *table, i64_t key, i64_t val, null_t *seed,
                     i64_t (*func)(i64_t key, i64_t val, null_t *seed, i64_t *tkey, i64_t *tval))
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

            continue;
        }

        table->count++;
        return func(key, val, seed, &keys[i], &vals[i]);
    }

    rehash_with(table);
    return ht_insert(table, key, val);
}

/*
 * Inserts new node or updates existing one.
 * Returns true if the node was updated, false if it was inserted.
 */
bool_t ht_update(ht_t *table, i64_t key, i64_t val)
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
                keys[i] = key;
                vals[i] = val;
                return true;
            }

            continue;
        }

        keys[i] = key;
        vals[i] = val;
        table->count++;
        return false;
    }

    rehash(table);
    return ht_insert(table, key, val);
}

/*
 * Does the same as ht_update, but uses a function to set the val of the bucket.
 */
// bool_t ht_update_with(ht_t *table, null_t *key, null_t *val, null_t *seed,
//                       null_t *(*func)(null_t *key, null_t *val, null_t *seed, bucket_t *bucket))
// {
//     // Table's size is always a power of 2
//     u64_t factor = table->size - 1,
//           index = table->hasher(key) & factor;
//     u32_t distance = 0;

//     while (distance < table->size)
//     {
//         if (table->buckets[index].state == STATE_OCCUPIED)
//         {
//             if (table->compare(table->buckets[index].key, key) == 0)
//             {
//                 func(key, val, seed, &table->buckets[index]);
//                 return true;
//             }
//         }
//         else
//         {
//             func(key, val, seed, &table->buckets[index]);
//             table->buckets[index].state = STATE_OCCUPIED;
//             table->count++;
//             return false;
//         }

//         if (table->buckets[index].distance < distance)
//         {
//             // Swap places
//             null_t *temp_key = table->buckets[index].key;
//             null_t *temp_val = table->buckets[index].val;
//             u64_t temp_distance = table->buckets[index].distance;

//             table->buckets[index].key = key;
//             table->buckets[index].val = val;
//             table->buckets[index].distance = distance;

//             key = temp_key;
//             val = temp_val;
//             distance = temp_distance;
//         }

//         index = (index + 1) & factor;
//         distance++;

//         // Rehash if the load factor is above 0.7
//         if (distance > table->size * 0.7)
//         {
//             rehash_with(table, seed, func);
//             // Start the insert operation from scratch, since the table has been resized and rehashed.
//             ht_update_with(table, key, val, seed, func);
//             return true;
//         }
//     }

//     panic("Hash table is full");
// }

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
    }

    return NULL_I64;
}

i64_t ht_next_key(ht_t *table, i64_t *index)
{
    i64_t *keys = as_vector_i64(&table->keys), i;
    while (*index < table->size)
    {
        if (keys[*index] != NULL_I64)
        {
            i = *index;
            (*index)++;
            return keys[i];
        }

        (*index)++;
    }

    return NULL_I64;
}
