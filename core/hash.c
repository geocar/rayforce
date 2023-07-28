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
#include "heap.h"
#include "util.h"

/*
 * Create a new hash table as a vector
 */
obj_t hash_table(u64_t size, u64_t bucket_size)
{
    u64_t i;
    obj_t obj;

    size = next_power_of_two_u64(size) * bucket_size;
    obj = vector(TYPE_I64, size);
    obj->mul = bucket_size;

    for (i = 0; i < size; i += bucket_size)
        as_i64(obj)[i] = NULL_I64;

    return obj;
}

nil_t ht_rehash(obj_t *obj, hash_f hash)
{
    u64_t i, l, size = (*obj)->len, mul = (*obj)->mul, key, val, factor, index;
    obj_t new_ht = hash_table(size * 2, mul);

    factor = new_ht->len - 1;

    for (i = 0; i < size * 2; i += mul)
        as_i64(new_ht)[i] = NULL_I64;

    for (i = 0; i < size; i += mul)
    {
        if (as_i64(*obj)[i] != NULL_I64)
        {
            key = as_i64(*obj)[i];
            val = as_i64(*obj)[i + 1];
            index = hash ? hash(key) & factor : key & factor;

            while (as_i64(new_ht)[i] != NULL_I64)
            {
                if (index == size * 2)
                    panic("ht is full!!");

                index = index + 1;
            }

            as_i64(new_ht)[index] = key;
            as_i64(new_ht)[index + 1] = val;
        }
    }

    drop(*obj);

    *obj = new_ht;
}

i64_t *ht_get(obj_t *obj, i64_t key)
{
    u64_t mul = (*obj)->mul;
    u64_t size = (*obj)->len / mul;

    while (true)
    {
        u64_t i = key & (size - 1);
        i64_t *b = as_i64(*obj) + i * mul;

        for (; i < size; i++, b += mul)
        {
            if (b[0] == NULL_I64 || b[0] == key)
                return b;
        }

        ht_rehash(obj, NULL);
        size = (*obj)->len / mul;
    }
}

i64_t *ht_get_with(obj_t *obj, i64_t key, hash_f hash, cmp_f cmp)
{
    u64_t mul = (*obj)->mul;
    u64_t size = (*obj)->len / mul;

    while (true)
    {
        u64_t i = hash(key) & (size - 1);
        i64_t *b = as_i64(*obj) + i * mul;

        for (; i < size; i++, b += mul)
        {
            if (b[0] == NULL_I64 || (cmp(b[0], key) == 0))
                return b;
        }

        ht_rehash(obj, hash);
        size = (*obj)->len / mul;
    }
}
