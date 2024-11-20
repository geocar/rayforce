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

#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include "rayforce.h"
#include "format.h"

// A compile time assertion check
#define CASSERT(predicate, file) _IMPL_CASSERT_LINE(predicate, __LINE__, file)
#define _IMPL_PASTE(a, b) a##b
#define _IMPL_CASSERT_LINE(predicate, line, file) \
    typedef char _IMPL_PASTE(assertion_failed_##file##_, line)[2 * !!(predicate) - 1];

#define UNUSED(x) (nil_t)(x)

#define LIKELY(x) __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)

#ifdef DEBUG

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

nil_t dump_stack(nil_t);

#define DEBUG_PRINT(fmt, ...)                \
    do {                                     \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
        fprintf(stderr, "\n");               \
        fflush(stderr);                      \
    } while (0)

#define DEBUG_ASSERT(x, fmt, ...)                      \
    {                                                  \
        if (!(x)) {                                    \
            fprintf(stderr, "-- ASSERTION FAILED:\n"); \
            fprintf(stderr, fmt, ##__VA_ARGS__);       \
            fprintf(stderr, "\n--\n");                 \
            fflush(stderr);                            \
            dump_stack();                              \
            assert(x);                                 \
        }                                              \
    }

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#define DEBUG_OBJ(o)                                    \
    {                                                   \
        obj_p _f = obj_fmt((o), B8_TRUE);               \
        DEBUG_PRINT("%.*s", (i32_t)_f->len, AS_C8(_f)); \
        drop_obj(_f);                                   \
    }

#else
#define DEBUG_PRINT(fmt, ...) (nil_t)0
#define DEBUG_ASSERT(x, fmt, ...) (nil_t)0
#define DEBUG_OBJ(o) (nil_t)0
#endif

#define PRINTBITS_N(x, n)                                                \
    {                                                                    \
        for (u64_t __i = n; __i; __i--, putchar('0' | ((x >> __i) & 1))) \
            ;                                                            \
        printf("\n");                                                    \
    }
#define PRINTBITS_32(x) PRINTBITS_N(x, 32)
#define PRINTBITS_64(x) PRINTBITS_N(x, 64)

#define TIMEIT(x)                                                           \
    {                                                                       \
        i64_t timer = clock();                                              \
        x;                                                                  \
        printf("%f\n", ((f64_t)(clock() - timer)) / CLOCKS_PER_SEC * 1000); \
    }

#define ENUM_KEY(x) \
    (x->mmod == MMOD_INTERNAL ? str_from_symbol(AS_LIST(x)[0]->i64) : AS_C8((obj_p)((str_p)x - RAY_PAGE_SIZE)))
#define ENUM_VAL(x) (x->mmod == MMOD_INTERNAL ? AS_LIST(x)[1] : x)

#define MAPLIST_KEY(x) (((obj_p)((str_p)x - RAY_PAGE_SIZE))->obj)
#define MAPLIST_VAL(x) (x)

#define UNWRAP_LIST(x)                            \
    {                                             \
        u64_t _i, _l;                             \
        obj_p _res;                               \
        _l = (x)->len;                            \
        for (_i = 0; _i < _l; _i++) {             \
            if (IS_ERROR(AS_LIST(x)[_i])) {       \
                _res = clone_obj(AS_LIST(x)[_i]); \
                drop_obj(x);                      \
                return _res;                      \
            }                                     \
        }                                         \
    }

b8_t is_valid(obj_p obj);
u32_t next_power_of_two_u32(u32_t n);
u64_t next_power_of_two_u64(u64_t n);

#endif  // UTIL_H
