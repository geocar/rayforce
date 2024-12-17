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

#ifndef OPS_H
#define OPS_H

#include "rayforce.h"

// Global null object to be referenced by all null objects.
extern struct obj_t __NULL_OBJECT;
// Return reference to the global null object.
#define NULL_OBJ (&__NULL_OBJECT)

// Function's attributes
#define FN_NONE 0
#define FN_LEFT_ATOMIC 1
#define FN_RIGHT_ATOMIC 2
#define FN_ATOMIC 4
#define FN_AGGR 8
#define FN_SPECIAL_FORM 16
#define FN_GROUP_MAP 32
#define FN_ATOMIC_MASK (FN_LEFT_ATOMIC | FN_RIGHT_ATOMIC | FN_ATOMIC)

// Object's attributes
#define ATTR_DISTINCT 1
#define ATTR_ASC 2
#define ATTR_DESC 4
#define ATTR_QUOTED 8
#define ATTR_PROTECTED 64

#define IS_INTERNAL(x) ((x)->mmod == MMOD_INTERNAL)
#define IS_EXTERNAL_SIMPLE(x) ((x)->mmod == MMOD_EXTERNAL_SIMPLE)
#define IS_EXTERNAL_COMPOUND(x) ((x)->mmod == MMOD_EXTERNAL_COMPOUND)
#define IS_EXTERNAL_SERIALIZED(x) ((x)->mmod == MMOD_EXTERNAL_SERIALIZED)

#define ALIGNUP(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN8(x) ((str_p)(((u64_t)x + 7) & ~7))
#define MTYPE2(x, y) ((u8_t)(x) | ((u8_t)(y) << 8))
#define ABSI64(x) ((x) == NULL_I64 ? 0 : (((x) < 0 ? -(x) : (x))))
#define ABSI32(x) ((x) == NULL_I32 ? 0 : (((x) < 0 ? -(x) : (x))))
#define ADDI32(x, y) (((x) == NULL_I32) ? (y) : ((y) == NULL_I32) ? NULL_I32 : (x) + (y))
#define ADDI64(x, y) (((x) == NULL_I64) ? (y) : ((y) == NULL_I64) ? NULL_I64 : (x) + (y))
#define ADDF64(x, y) (ops_is_nan(x) ? (y) : ops_is_nan(y) ? (x) : (x) + (y))
#define SUBI32(x, y) (((x) == NULL_I32) ? -(y) : ((y) == NULL_I32) ? (x) : (x) - (y))
#define SUBI64(x, y) ((x == NULL_I64) ? -(y) : (y == NULL_I64) ? x : (x) - (y))
#define SUBF64(x, y) (ops_is_nan(x) ? -(y) : ops_is_nan(y) ? (x) : (x) - (y))
#define MULI32(x, y) (((x) == NULL_I32 || (y) == NULL_I32) ? NULL_I32 : (x) * (y))
#define MULI64(x, y) (((x) == NULL_I64 || (y) == NULL_I64) ? NULL_I64 : (x) * (y))
#define MULF64(x, y) ((x) * (y))
#define DIVI32(x, y) (((y) == 0) ? NULL_I32 : (((x) == NULL_I32 || (y) == NULL_I32) ? NULL_F64 : ((x) / (y))))
#define DIVI64(x, y) (((y) == 0) ? NULL_I64 : (((x) == NULL_I64 || (y) == NULL_I64) ? NULL_F64 : ((x) / (y))))
#define DIVF64(x, y) ((i64_t)((x) / (y)))
#define FDIVI32(x, y) (((x) == NULL_I32 || (y) == NULL_I32) ? NULL_F64 : ((f64_t)(x) / (f64_t)(y)))
#define FDIVI64(x, y) (((x) == NULL_I64 || (y) == NULL_I64) ? NULL_F64 : ((f64_t)(x) / (f64_t)(y)))
#define FDIVF64(x, y) ((x) / (y))
#define MODI32(x, y) (((y) == 0) ? NULL_I32 : (((x) == NULL_I32 || (y) == NULL_I32) ? NULL_I32 : ((x) % (y))))
#define MODI64(x, y) \
    (((y) == 0) ? NULL_I64 : (((x) == NULL_I64 || (y) == NULL_I64) ? NULL_I64 : (((i64_t)(x)) % ((i64_t)(y)))))
#define MODF64(x, y) ((x) - (y) * ((i64_t)((x) / (y))))
#define MAXI32(x, y) ((x) > (y) ? (x) : (y))
#define MAXI64(x, y) ((x) > (y) ? (x) : (y))
#define MAXF64(x, y) ((x) > (y) ? (x) : (y))
#define MINI32(x, y) ((x) < (y) ? (x) : (y))
#define MINI64(x, y) (((y) == NULL_I64) || (((x) != NULL_I64) && ((x) < (y))) ? (x) : (y))
#define MINF64(x, y) ((x) < (y) ? (x) : (y))
#define ROTI32(x, y) (((x) << (y)) | ((x) >> (32 - (y))))
#define ROTI64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))
#define ROUNDF64(x) ((x) >= 0.0 ? (i64_t)((x) + 0.5) : (i64_t)((x) - 0.5))
#define FLOORF64(x) ((x) >= 0.0 ? (i64_t)(x) : (i64_t)((x) - 1.0))
#define CEILF64(x) ((x) >= 0.0 ? (i64_t)((x) + 1.0) : (i64_t)(x))
#define XBARI32(x, y) (((y) == 0) ? NULL_I32 : (((x) == NULL_I32 || (y) == NULL_I32) ? NULL_I32 : ((x) / (y)) * (y)))
#define XBARI64(x, y) (((x) == NULL_I64 || (y) == NULL_I64) ? NULL_I64 : ((x) / (y)) * (y))
#define XBARF64(x, y) ((i64_t)(((x) / (y)) * y))

// Function types
typedef u64_t (*hash_f)(i64_t, raw_p);
typedef i64_t (*cmp_f)(i64_t, i64_t, raw_p);
typedef obj_p (*unary_f)(obj_p);
typedef obj_p (*binary_f)(obj_p, obj_p);
typedef obj_p (*vary_f)(obj_p *, u64_t);

typedef enum { ERROR_TYPE_OS, ERROR_TYPE_SYS, ERROR_TYPE_SOCK } os_ray_error_type_t;

b8_t ops_as_b8(obj_p x);
b8_t ops_is_nan(f64_t x);
b8_t ops_is_prime(u64_t x);
u64_t ops_next_prime(u64_t x);
u64_t ops_rand_u64(nil_t);
u64_t ops_count(obj_p x);
u64_t ops_count_ones(b8_t *mask, u64_t len);
u64_t ops_rank(obj_p *x, u64_t n);
b8_t ops_eq_idx(obj_p a, i64_t ai, obj_p b, i64_t bi);
obj_p index_find_i64(i64_t x[], u64_t xl, i64_t y[], u64_t yl);
obj_p ops_where(b8_t *mask, u64_t n);
obj_p sys_error(os_ray_error_type_t, lit_p msg);

#endif  // OPS_H
