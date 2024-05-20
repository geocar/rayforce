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
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "string.h"
#include "hash.h"
#include "rayforce.h"
#include "heap.h"
#include "util.h"
#include "eval.h"
#include "error.h"
#include "atomic.h"

/*
 * Simplefied version of murmurhash
 */
u64_t str_hash(lit_p key, u64_t len)
{
    u64_t i, k, k1;
    u64_t hash = 0x1234ABCD1234ABCD;
    u64_t c1 = 0x87c37b91114253d5ULL;
    u64_t c2 = 0x4cf5ad432745937fULL;
    const int r1 = 31;
    const int r2 = 27;
    const u64_t m = 5ULL;
    const u64_t n = 0x52dce729ULL;

    // Process each 8-byte block of the key
    for (i = 0; i + 7 < len; i += 8)
    {
        k = (u64_t)key[i] |
            ((u64_t)key[i + 1] << 8) |
            ((u64_t)key[i + 2] << 16) |
            ((u64_t)key[i + 3] << 24) |
            ((u64_t)key[i + 4] << 32) |
            ((u64_t)key[i + 5] << 40) |
            ((u64_t)key[i + 6] << 48) |
            ((u64_t)key[i + 7] << 56);

        k *= c1;
        k = (k << r1) | (k >> (64 - r1));
        k *= c2;

        hash ^= k;
        hash = ((hash << r2) | (hash >> (64 - r2))) * m + n;
    }

    // Process the tail of the data
    k1 = 0;
    switch (len & 7)
    {
    case 7:
        k1 ^= ((u64_t)key[i + 6]) << 48; // fall through
    case 6:
        k1 ^= ((u64_t)key[i + 5]) << 40; // fall through
    case 5:
        k1 ^= ((u64_t)key[i + 4]) << 32; // fall through
    case 4:
        k1 ^= ((u64_t)key[i + 3]) << 24; // fall through
    case 3:
        k1 ^= ((u64_t)key[i + 2]) << 16; // fall through
    case 2:
        k1 ^= ((u64_t)key[i + 1]) << 8; // fall through
    case 1:
        k1 ^= ((u64_t)key[i]);
        k1 *= c1;
        k1 = (k1 << r1) | (k1 >> (64 - r1));
        k1 *= c2;
        hash ^= k1;
    }

    // Finalize the hash
    hash ^= len;
    hash ^= (hash >> 33);
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= (hash >> 33);
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= (hash >> 33);

    return hash;
}

obj_p ht_oa_create(u64_t size, i8_t vals)
{
    u64_t i;
    obj_p k, v;

    size = next_power_of_two_u64(size);
    k = vector(TYPE_I64, size);

    if (vals >= 0)
        v = vector(vals, size);
    else
        v = NULL_OBJ;

    for (i = 0; i < size; i++)
        as_i64(k)[i] = NULL_I64;

    return dict(k, v);
}

nil_t ht_oa_rehash(obj_p *obj, hash_f hash, raw_p seed)
{
    u64_t i, j, size, key, factor;
    obj_p new_obj;
    i8_t type;
    i64_t *orig_keys, *new_keys, *orig_vals = NULL, *new_vals = NULL;

    size = as_list(*obj)[0]->len;
    orig_keys = as_i64(as_list(*obj)[0]);

    type = is_null(as_list(*obj)[1]) ? -1 : as_list(*obj)[1]->type;

    if (type > -1)
        orig_vals = as_i64(as_list(*obj)[1]);

    new_obj = ht_oa_create(size * 2, type);

    factor = as_list(new_obj)[0]->len - 1;
    new_keys = as_i64(as_list(new_obj)[0]);

    if (type > -1)
        new_vals = as_i64(as_list(new_obj)[1]);

    for (i = 0; i < size; i++)
    {
        if (orig_keys[i] != NULL_I64)
        {
            key = orig_keys[i];

            // Recalculate the index for the new table
            j = hash ? hash(key, seed) & factor : (u64_t)key & factor;

            while (new_keys[j] != NULL_I64)
            {
                j++;

                if (j == size)
                    panic("ht is full");
            }

            new_keys[j] = key;

            if (type > -1)
                new_vals[j] = orig_vals[i];
        }
    }

    drop_obj(*obj);
    *obj = new_obj;
}

i64_t ht_oa_tab_next(obj_p *obj, i64_t key)
{
    u64_t i, size;
    i64_t *keys;

    size = as_list(*obj)[0]->len;
    keys = as_i64(as_list(*obj)[0]);

next:
    for (i = (u64_t)key & (size - 1); i < size; i++)
    {
        if ((keys[i] == NULL_I64) || (keys[i] == key))
            return i;
    }

    ht_oa_rehash(obj, NULL, NULL);
    size = as_list(*obj)[0]->len;
    keys = as_i64(as_list(*obj)[0]);

    goto next;
}

i64_t ht_oa_tab_next_with(obj_p *obj, i64_t key, hash_f hash, cmp_f cmp, raw_p seed)
{
    u64_t i, size;
    i64_t *keys;

    size = as_list(*obj)[0]->len;
    keys = as_i64(as_list(*obj)[0]);

next:
    for (i = hash(key, seed) & (size - 1); i < size; i++)
    {
        if (keys[i] == NULL_I64 || cmp(keys[i], key, seed) == 0)
            return i;
    }

    ht_oa_rehash(obj, hash, seed);

    size = as_list(*obj)[0]->len;
    keys = as_i64(as_list(*obj)[0]);

    goto next;
}

i64_t ht_oa_tab_get(obj_p obj, i64_t key)
{
    u64_t i, size;
    i64_t *keys;

    size = as_list(obj)[0]->len;
    keys = as_i64(as_list(obj)[0]);

    for (i = (u64_t)key & (size - 1); i < size; i++)
    {
        if (keys[i] == NULL_I64)
            return NULL_I64;
        else if (keys[i] == key)
            return i;
    }

    return NULL_I64;
}

i64_t ht_oa_tab_get_with(obj_p obj, i64_t key, hash_f hash, cmp_f cmp, raw_p seed)
{
    u64_t i, size;
    i64_t *keys;

    size = as_list(obj)[0]->len;
    keys = as_i64(as_list(obj)[0]);

    for (i = hash(key, seed) & (size - 1); i < size; i++)
    {
        if (keys[i] == NULL_I64)
            return NULL_I64;
        else if (cmp(keys[i], key, seed) == 0)
            return i;
    }

    return NULL_I64;
}

u64_t hash_index_obj(obj_p obj)
{
    u64_t hash, len, i, c;
    str_p str;

    switch (obj->type)
    {
    case -TYPE_I64:
    case -TYPE_SYMBOL:
    case -TYPE_TIMESTAMP:
        return (u64_t)obj->i64;
    case -TYPE_F64:
        return (u64_t)obj->f64;
    case -TYPE_GUID:
        return hash_index_u64(*(u64_t *)as_guid(obj), *((u64_t *)as_guid(obj) + 1));
    case TYPE_C8:
        str = as_string(obj);
        hash = 5381;
        while ((c = *str++))
            hash = ((hash << 5) + hash) + c;

        return hash;
    case TYPE_I64:
    case TYPE_SYMBOL:
    case TYPE_TIMESTAMP:
        len = obj->len;
        for (i = 0, hash = 0xcbf29ce484222325ull; i < len; i++)
            hash = hash_index_u64((u64_t)as_i64(obj)[i], hash);
        return hash;
    default:
        panic("hash: unsupported type: %d", obj->type);
    }
}

ht_bk_p ht_bk_create(u64_t size)
{
    u64_t i;
    ht_bk_p ht;

    ht = (ht_bk_p)heap_alloc(sizeof(struct ht_bk_t) + size * sizeof(bucket_p));
    if (ht == NULL)
        return NULL;

    ht->size = size;
    ht->count = 0;
    for (i = 0; i < size; i++)
        ht->table[i] = NULL;

    return ht;
}

nil_t ht_bk_destroy(ht_bk_p ht)
{
    u64_t i;
    bucket_p current, next;

    // free buckets
    for (i = 0; i < ht->size; i++)
    {
        current = ht->table[i];
        while (current != NULL)
        {
            next = current->next;
            heap_free(current);
            current = next;
        }
    }

    heap_free(ht);
}

nil_t ht_bk_rehash(ht_bk_p *ht, u64_t new_size)
{
    u64_t i;
    bucket_p bucket;
    ht_bk_p new_ht = ht_bk_create(new_size);

    if (new_ht == NULL)
    {
        // Handle memory allocation failure
        fprintf(stderr, "Memory allocation failed during rehash.\n");
        return;
    }

    // Rehash all elements from the old table to the new table
    for (i = 0; i < (*ht)->size; ++i)
    {
        bucket = (*ht)->table[i];
        while (bucket != NULL)
        {
            ht_bk_insert(new_ht, bucket->key, bucket->val);
            bucket = bucket->next;
        }
    }

    // Free the old hash table
    ht_bk_destroy(*ht);

    // Update the pointer to point to the new hash table
    *ht = new_ht;
}

i64_t ht_bk_insert(ht_bk_p ht, i64_t key, i64_t val)
{
    i64_t index;
    bucket_p bucket;

    // Check the load factor and rehash if necessary
    if ((ht->count + 1) > (ht->size * 0.75))
        ht_bk_rehash(&ht, ht->size * 2);

    index = key % ht->size;

    bucket = ht->table[index];

    if (bucket == NULL)
    {
        bucket = (bucket_p)heap_alloc(sizeof(struct bucket_t));
        if (bucket == NULL)
            return NULL_I64;

        bucket->key = key;
        bucket->val = val;
        bucket->next = NULL;
        ht->table[index] = bucket;
        ht->count++;

        return val;
    }

    while (bucket != NULL)
    {
        if (bucket->key == key)
            return bucket->val;

        bucket = bucket->next;
    }

    // Key does not exist, insert a new bucket
    bucket = (bucket_p)heap_alloc(sizeof(struct bucket_t));
    if (bucket == NULL)
        return NULL_I64;

    bucket->key = key;
    bucket->val = val;
    bucket->next = ht->table[index];
    ht->table[index] = bucket;
    ht->count++;

    return val;
}

i64_t ht_bk_insert_with(ht_bk_p ht, i64_t key, i64_t val, hash_f hash, cmp_f cmp, raw_p seed)
{
    i64_t index;
    bucket_p bucket;

    index = hash(key, seed) % ht->size;

    bucket = ht->table[index];

    if (bucket == NULL)
    {
        bucket = (bucket_p)heap_alloc(sizeof(struct bucket_t));
        if (bucket == NULL)
            return NULL_I64;

        bucket->key = key;
        bucket->val = val;
        bucket->next = NULL;
        ht->table[index] = bucket;
        ht->count++;

        return val;
    }

    while (bucket != NULL)
    {
        if (cmp(bucket->key, key, seed) == 0)
            return bucket->val;

        bucket = bucket->next;
    }

    // Key does not exist, insert a new bucket
    bucket = (bucket_p)heap_alloc(sizeof(struct bucket_t));
    if (bucket == NULL)
        return NULL_I64;

    bucket->key = key;
    bucket->val = val;
    bucket->next = ht->table[index];
    ht->table[index] = bucket;
    ht->count++;

    return val;
}

i64_t ht_bk_insert_par(ht_bk_p ht, i64_t key, i64_t val)
{
    i64_t index;
    bucket_p new_bucket, current_bucket, b;

    // if ((atomic_load(&ht->count) + 1) > (ht->size * 0.75)) {
    //     ht_bk_rehash(&ht, ht->size * 2);
    // }

    index = key % ht->size;

    new_bucket = (bucket_p)heap_alloc(sizeof(struct bucket_t));
    if (new_bucket == NULL)
        return NULL_I64;

    new_bucket->key = key;
    new_bucket->val = val;

    for (;;)
    {
        current_bucket = __atomic_load_n(&ht->table[index], __ATOMIC_ACQUIRE);
        b = current_bucket;

        while (b != NULL)
        {
            if (b->key == key)
            {
                heap_free(new_bucket);
                return b->val; // Key already exists
            }

            b = __atomic_load_n(&b->next, __ATOMIC_ACQUIRE);
        }

        new_bucket->next = current_bucket;
        if (__atomic_compare_exchange_n(&ht->table[index], &current_bucket, new_bucket, 1, __ATOMIC_RELEASE, __ATOMIC_RELAXED))
        {
            __atomic_fetch_add(&ht->count, 1, __ATOMIC_RELAXED);
            return val;
        }
    }
}

i64_t ht_bk_insert_str_par(ht_bk_p ht, lit_p str, u64_t len, i64_t id, str_p *key)
{
    i64_t index;
    str_p intr;
    bucket_p new_bucket, current_bucket, b;

    index = str_hash(str, len) % ht->size;
    intr = heap_intern(len + 1);

    new_bucket = (bucket_p)heap_alloc(sizeof(struct bucket_t));
    if (new_bucket == NULL)
        return NULL_I64;

    memcpy(intr, str, len);
    intr[len] = '\0';

    new_bucket->key = (i64_t)intr;
    new_bucket->val = id;

    for (;;)
    {
        current_bucket = __atomic_load_n(&ht->table[index], __ATOMIC_ACQUIRE);
        b = current_bucket;

        while (b != NULL)
        {
            if (strncmp((str_p)b->key, str, len) == 0)
            {
                heap_untern(len + 1);
                heap_free(new_bucket);
                return b->val;
            }

            b = __atomic_load_n(&b->next, __ATOMIC_ACQUIRE);
        }

        new_bucket->next = current_bucket;
        if (__atomic_compare_exchange(&ht->table[index], &current_bucket, &new_bucket, 1, __ATOMIC_RELEASE, __ATOMIC_RELAXED))
        {
            __atomic_fetch_add(&ht->count, 1, __ATOMIC_RELAXED);
            *key = intr;

            return id;
        }
    }
}

i64_t ht_bk_insert_str(ht_bk_p ht, lit_p str, u64_t len, i64_t id, str_p *key)
{
    i64_t index;
    bucket_p bucket;
    str_p intr;

    index = str_hash(str, len) % ht->size;

    bucket = ht->table[index];

    if (bucket == NULL)
    {
        bucket = (bucket_p)heap_alloc(sizeof(struct bucket_t));
        if (bucket == NULL)
            return NULL_I64;

        intr = heap_intern(len + 1);
        memcpy(intr, str, len);
        intr[len] = '\0';

        bucket->key = (i64_t)intr;
        bucket->val = id;
        bucket->next = NULL;
        ht->table[index] = bucket;
        ht->count++;

        *key = intr;

        return id;
    }

    while (bucket != NULL)
    {
        if (strncmp((str_p)bucket->key, str, len) == 0)
            return bucket->val;

        bucket = bucket->next;
    }

    // Key does not exist, insert a new bucket
    bucket = (bucket_p)heap_alloc(sizeof(struct bucket_t));
    if (bucket == NULL)
        return NULL_I64;

    intr = heap_intern(len + 1);
    memcpy(intr, str, len);
    intr[len] = '\0';

    bucket->key = (i64_t)intr;
    bucket->val = id;
    bucket->next = ht->table[index];
    ht->table[index] = bucket;
    ht->count++;

    *key = intr;

    return id;
}

i64_t ht_bk_get(ht_bk_p ht, i64_t key)
{
    u64_t index = key % ht->size;
    bucket_p current = ht->table[index];

    while (current != NULL)
    {
        if (current->key == key)
            return current->val;

        current = current->next;
    }

    return NULL_I64;
}

u64_t hash_kmh(i64_t key, nil_t *seed)
{
    unused(seed);
    return (key * 6364136223846793005ull) >> 32;
}

u64_t hash_fnv1a(i64_t key, nil_t *seed)
{
    unused(seed);
    u64_t hash = 14695981039346656037ull;
    i32_t i;

    for (i = 0; i < 8; i++)
    {
        u8_t byte = (key >> (i * 8)) & 0xff;
        hash ^= byte;
        hash *= 1099511628211ull;
    }

    return hash;
}

u64_t hash_guid(i64_t a, raw_p seed)
{
    unused(seed);
    guid_t *g = (guid_t *)a;
    u64_t upper_part, lower_part;

    // Cast the first and second halves of the GUID to u64_t
    memcpy(&upper_part, g->buf, sizeof(u64_t));
    memcpy(&lower_part, g->buf + 8, sizeof(u64_t));

    // Combine the two parts
    return upper_part ^ lower_part;
}

u64_t hash_obj(i64_t a, raw_p seed)
{
    unused(seed);
    return hash_index_obj((obj_p)a);
}

i64_t hash_cmp_i64(i64_t a, i64_t b, raw_p seed)
{
    unused(seed);
    return (a < b) ? -1 : ((a > b) ? 1 : 0);
}

i64_t hash_cmp_obj(i64_t a, i64_t b, raw_p seed)
{
    unused(seed);
    return cmp_obj((obj_p)a, (obj_p)b);
}

i64_t hash_cmp_guid(i64_t a, i64_t b, raw_p seed)
{
    unused(seed);
    guid_t *g1 = (guid_t *)a, *g2 = (guid_t *)b;
    return memcmp(g1->buf, g2->buf, sizeof(guid_t));
}
