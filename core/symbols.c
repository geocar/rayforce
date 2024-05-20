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
#include "symbols.h"
#include "rayforce.h"
#include "heap.h"
#include "util.h"
#include "runtime.h"
#include "ops.h"
#include "atomic.h"

#define SYMBOLS_HT_SIZE 1024 * 1024

symbols_p symbols_create(nil_t)
{
    symbols_p symbols = (symbols_p)heap_mmap(sizeof(struct symbols_t));

    symbols->count = 0;
    symbols->str_to_id = ht_bk_create(SYMBOLS_HT_SIZE);
    symbols->id_to_str = ht_bk_create(SYMBOLS_HT_SIZE);

    return symbols;
}

nil_t symbols_destroy(symbols_p symbols)
{
    ht_bk_destroy(symbols->str_to_id);
    ht_bk_destroy(symbols->id_to_str);

    heap_unmap(symbols, sizeof(struct symbols_t));
}

i64_t symbols_intern(lit_p s, u64_t len)
{
    str_p str;
    i64_t id, new_id;
    symbols_p symbols = runtime_get()->symbols;

    // insert new symbol
    new_id = __atomic_load_n(&symbols->count, __ATOMIC_RELAXED);

    id = ht_bk_insert_str_par(symbols->str_to_id, s, len, new_id, &str);

    // Already exists
    if (new_id != id)
        return id;

    // insert id into id_to_str
    ht_bk_insert_par(symbols->id_to_str, new_id, (i64_t)str);

    __atomic_fetch_add(&symbols->count, 1, __ATOMIC_RELAXED);

    return new_id;
}

str_p symbols_strof(i64_t key)
{
    symbols_p symbols = runtime_get()->symbols;
    i64_t sym = ht_bk_get(symbols->id_to_str, key);

    if (sym == NULL_I64)
        return (str_p) "";

    return (str_p)sym;
}

u64_t symbols_count(symbols_p symbols)
{
    return symbols->count;
}
