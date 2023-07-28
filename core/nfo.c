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

#include "nfo.h"
#include "string.h"
#include "heap.h"
#include "ops.h"
#include "util.h"

nfo_t nfo_new(str_t filename, str_t lambda)
{
    nfo_t nfo = {
        .filename = filename,
        .lambda = lambda,
        .spans = hash_table(32, 2),
    };

    return nfo;
}

nil_t nfo_insert(nfo_t *nfo, i64_t index, span_t span)
{
    i64_t *b = ht_get(&nfo->spans, index);
    b[0] = index;
    memcpy(b + 1, &span, sizeof(span_t));
}

span_t nfo_get(nfo_t *nfo, i64_t index)
{
    i64_t *b = ht_get(&nfo->spans, index);

    if (b[0] == NULL_I64)
        return (span_t){0};

    span_t span;
    memcpy(&span, b + 1, sizeof(span_t));

    return span;
}

nil_t nfo_free(nfo_t *nfo)
{
    drop(nfo->spans);
}