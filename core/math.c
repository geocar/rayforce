/*
 *   Copyright (c) 2023 Anton Kundenko <singaraiona@gmail.com>
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, modlicense, and/or sell
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

#include <math.h>
#include <time.h>
#include "math.h"
#include "util.h"
#include "heap.h"
#include "ops.h"
#include "error.h"
#include "aggr.h"
#include "pool.h"
#include "sort.h"
#include "order.h"
#include "runtime.h"

#define __DISPATCH_BINOP(x, y, op, ot, ov)                       \
    _Generic((x),                                                \
        i32_t: _Generic((y),                                     \
            i32_t: ov = op(x, y),                                \
            i64_t: ov = i64_to_##ot(__BINOP_I32_I64(x, y, op)),  \
            f64_t: ov = f64_to_##ot(__BINOP_I32_F64(x, y, op))), \
        i64_t: _Generic((y),                                     \
            i32_t: ov = i64_to_##ot(__BINOP_I64_I32(x, y, op)),  \
            i64_t: ov = op(x, y),                                \
            f64_t: ov = f64_to_##ot(__BINOP_I64_F64(x, y, op))), \
        f64_t: _Generic((y),                                     \
            i32_t: ov = f64_to_##ot(__BINOP_F64_I32(x, y, op)),  \
            i64_t: ov = f64_to_##ot(__BINOP_F64_I64(x, y, op)),  \
            f64_t: ov = op(x, y)))

#define __BINOP_A_V(x, y, lt, rt, ot, op, ln, of, ov)            \
    ({                                                           \
        rt##_t *$rhs;                                            \
        __INNER_##ot *$out;                                      \
        $rhs = __AS_##rt(y) + of;                                \
        $out = __AS_##ot(ov) + of;                               \
        for (u64_t $i = 0; $i < ln; $i++)                        \
            __DISPATCH_BINOP(x->lt, $rhs[$i], op, ot, $out[$i]); \
        NULL_OBJ;                                                \
    })

#define __BINOP_V_A(x, y, lt, rt, ot, op, ln, of, ov)            \
    ({                                                           \
        lt##_t *$lhs;                                            \
        __INNER_##ot *$out;                                      \
        $lhs = __AS_##lt(x) + of;                                \
        $out = __AS_##ot(ov) + of;                               \
        for (u64_t $i = 0; $i < ln; $i++)                        \
            __DISPATCH_BINOP($lhs[$i], y->rt, op, ot, $out[$i]); \
        NULL_OBJ;                                                \
    })

#define __BINOP_V_V(x, y, lt, rt, ot, op, ln, of, ov)               \
    ({                                                              \
        lt##_t *$lhs;                                               \
        rt##_t *$rhs;                                               \
        __INNER_##ot *$out;                                         \
        $lhs = __AS_##lt(x) + of;                                   \
        $rhs = __AS_##rt(y) + of;                                   \
        $out = __AS_##ot(ov) + of;                                  \
        for (u64_t $i = 0; $i < ln; $i++)                           \
            __DISPATCH_BINOP($lhs[$i], $rhs[$i], op, ot, $out[$i]); \
        NULL_OBJ;                                                   \
    })

i8_t infer_math_type(obj_p x, obj_p y) {
    switch (MTYPE2(ABSI8(x->type), ABSI8(y->type))) {
        case MTYPE2(TYPE_I32, TYPE_I32):
            return TYPE_I32;
        case MTYPE2(TYPE_I32, TYPE_I64):
        case MTYPE2(TYPE_I64, TYPE_I32):
            return TYPE_I64;
        case MTYPE2(TYPE_I32, TYPE_F64):
        case MTYPE2(TYPE_F64, TYPE_I32):
            return TYPE_F64;
        case MTYPE2(TYPE_I32, TYPE_DATE):
        case MTYPE2(TYPE_DATE, TYPE_I32):
        case MTYPE2(TYPE_DATE, TYPE_I64):
        case MTYPE2(TYPE_I64, TYPE_DATE):
            return TYPE_DATE;
        case MTYPE2(TYPE_I32, TYPE_TIME):
        case MTYPE2(TYPE_TIME, TYPE_I32):
            return TYPE_TIME;
        case MTYPE2(TYPE_I32, TYPE_TIMESTAMP):
        case MTYPE2(TYPE_TIMESTAMP, TYPE_I32):
        case MTYPE2(TYPE_I64, TYPE_TIMESTAMP):
        case MTYPE2(TYPE_TIMESTAMP, TYPE_I64):
            return TYPE_TIMESTAMP;
        case MTYPE2(TYPE_I64, TYPE_I64):
            return TYPE_I64;
        case MTYPE2(TYPE_I64, TYPE_F64):
        case MTYPE2(TYPE_F64, TYPE_I64):
        case MTYPE2(TYPE_F64, TYPE_F64):
            return TYPE_F64;
        default:
            return TYPE_I64;
    }
}

obj_p ray_add_partial(obj_p x, obj_p y, u64_t len, u64_t offset, obj_p out) {
    i64_t vi64;

    switch (MTYPE2(x->type, y->type)) {
        case MTYPE2(-TYPE_I32, -TYPE_I32):
            return i32(ADDI32(x->i32, y->i32));
        case MTYPE2(-TYPE_I32, -TYPE_I64):
            return i64(ADDI64(i32_to_i64(x->i32), y->i64));
        case MTYPE2(-TYPE_I32, -TYPE_DATE):
            return adate(ADDI32(x->i32, y->i32));
        case MTYPE2(-TYPE_I32, -TYPE_TIME):
            return atime(ADDI32(x->i32, y->i32));
        case MTYPE2(-TYPE_I32, -TYPE_TIMESTAMP):
            vi64 = i32_to_i64(x->i32);
            return timestamp(ADDI64(vi64, y->i64));
        case MTYPE2(-TYPE_I64, -TYPE_I64):
            return i64(ADDI64(x->i64, y->i64));
        case MTYPE2(-TYPE_I64, -TYPE_F64):
            return f64(ADDF64(x->i64, y->f64));
        case MTYPE2(-TYPE_I64, -TYPE_I32):
            return i64(ADDI64(x->i64, i32_to_i64(y->i32)));
        case MTYPE2(-TYPE_I64, -TYPE_DATE):
            vi64 = i32_to_i64(y->i32);
            return adate(i64_to_i32(ADDI64(x->i64, vi64)));
        case MTYPE2(-TYPE_I64, -TYPE_TIME):
            vi64 = i32_to_i64(y->i32);
            return atime(i64_to_i32(ADDI64(x->i64, vi64)));
        case MTYPE2(-TYPE_I64, -TYPE_TIMESTAMP):
            return timestamp(ADDI64(x->i64, y->i64));
        case MTYPE2(-TYPE_F64, -TYPE_F64):
            return f64(ADDF64(x->f64, y->f64));
        case MTYPE2(-TYPE_F64, -TYPE_I64):
            return f64(ADDF64(x->f64, (f64_t)y->i64));
        case MTYPE2(-TYPE_DATE, -TYPE_I64):
            return adate(i64_to_i32(ADDI64(i32_to_i64(x->i32), y->i64)));
        case MTYPE2(-TYPE_DATE, -TYPE_I32):
            return adate(ADDI32(x->i32, y->i32));
        case MTYPE2(-TYPE_TIME, -TYPE_I64):
            vi64 = i32_to_i64(y->i32);
            return atime(i64_to_i32(ADDI64(vi64, y->i64)));
        case MTYPE2(-TYPE_TIME, -TYPE_I32):
            return atime(ADDI32(x->i32, y->i32));
        case MTYPE2(-TYPE_TIMESTAMP, -TYPE_I64):
            return timestamp(ADDI64(x->i64, y->i64));
        case MTYPE2(-TYPE_I32, TYPE_I32):
            return __BINOP_A_V(x, y, i32, i32, i32, ADDI32, len, offset, out);
        case MTYPE2(-TYPE_I32, TYPE_I64):
            return __BINOP_A_V(x, y, i32, i64, i64, ADDI64, len, offset, out);
        case MTYPE2(-TYPE_DATE, TYPE_I64):
            return __BINOP_A_V(x, y, i32, i64, date, ADDI64, len, offset, out);
        case MTYPE2(-TYPE_TIME, TYPE_I64):
            return __BINOP_A_V(x, y, i32, i64, time, ADDI64, len, offset, out);
        case MTYPE2(-TYPE_I32, TYPE_F64):
            return __BINOP_A_V(x, y, i32, f64, f64, ADDF64, len, offset, out);
        case MTYPE2(-TYPE_I64, TYPE_I32):
            return __BINOP_A_V(x, y, i64, i32, i64, ADDI64, len, offset, out);
        case MTYPE2(-TYPE_I32, TYPE_DATE):
            return __BINOP_A_V(x, y, i32, i32, date, ADDI32, len, offset, out);
        case MTYPE2(-TYPE_I32, TYPE_TIME):
            return __BINOP_A_V(x, y, i32, i32, time, ADDI32, len, offset, out);
        case MTYPE2(-TYPE_I32, TYPE_TIMESTAMP):
            return __BINOP_A_V(x, y, i32, i64, timestamp, ADDI64, len, offset, out);
        case MTYPE2(-TYPE_I64, TYPE_DATE):
            return __BINOP_A_V(x, y, i64, i32, date, ADDI64, len, offset, out);
        case MTYPE2(-TYPE_I64, TYPE_TIME):
            return __BINOP_A_V(x, y, i64, i32, time, ADDI64, len, offset, out);
        case MTYPE2(-TYPE_I64, TYPE_I64):
            return __BINOP_A_V(x, y, i64, i64, i64, ADDI64, len, offset, out);
        case MTYPE2(-TYPE_I64, TYPE_F64):
            return __BINOP_A_V(x, y, i64, f64, f64, ADDF64, len, offset, out);
        case MTYPE2(-TYPE_F64, TYPE_F64):
            return __BINOP_A_V(x, y, f64, f64, f64, ADDF64, len, offset, out);
        case MTYPE2(-TYPE_F64, TYPE_I64):
            return __BINOP_A_V(x, y, f64, i64, f64, ADDF64, len, offset, out);
        case MTYPE2(TYPE_I64, -TYPE_I64):
            return __BINOP_V_A(x, y, i64, i64, i64, ADDI64, len, offset, out);
        case MTYPE2(TYPE_I64, -TYPE_F64):
            return __BINOP_V_A(x, y, i64, f64, f64, ADDF64, len, offset, out);
        case MTYPE2(TYPE_I64, TYPE_I64):
            return __BINOP_V_V(x, y, i64, i64, i64, ADDI64, len, offset, out);
        case MTYPE2(TYPE_I64, TYPE_F64):
            return __BINOP_V_V(x, y, i64, f64, f64, ADDF64, len, offset, out);
        case MTYPE2(TYPE_F64, -TYPE_F64):
            return __BINOP_V_A(x, y, f64, f64, f64, ADDF64, len, offset, out);
        case MTYPE2(TYPE_F64, -TYPE_I64):
            return __BINOP_V_A(x, y, f64, i64, f64, ADDF64, len, offset, out);
        case MTYPE2(TYPE_F64, TYPE_F64):
            return __BINOP_V_V(x, y, f64, f64, f64, ADDF64, len, offset, out);
        case MTYPE2(TYPE_F64, TYPE_I64):
            return __BINOP_V_V(x, y, f64, i64, f64, ADDF64, len, offset, out);
        case MTYPE2(TYPE_DATE, -TYPE_I64):
            return __BINOP_V_A(x, y, i32, i64, date, ADDI64, len, offset, out);
        case MTYPE2(TYPE_DATE, TYPE_I32):
            return __BINOP_V_A(x, y, i32, i32, date, ADDI32, len, offset, out);
        case MTYPE2(TYPE_DATE, TYPE_I64):
            return __BINOP_V_V(x, y, i32, i64, date, ADDI64, len, offset, out);
        case MTYPE2(TYPE_TIME, -TYPE_I64):
            return __BINOP_V_A(x, y, i32, i64, time, ADDI64, len, offset, out);
        case MTYPE2(TYPE_TIME, TYPE_I32):
            return __BINOP_V_A(x, y, i32, i32, time, ADDI32, len, offset, out);
        case MTYPE2(TYPE_TIME, TYPE_I64):
            return __BINOP_V_V(x, y, i32, i64, time, ADDI64, len, offset, out);
        case MTYPE2(TYPE_TIMESTAMP, -TYPE_I64):
            return __BINOP_V_A(x, y, i64, i64, timestamp, ADDI64, len, offset, out);
        case MTYPE2(TYPE_TIMESTAMP, TYPE_I64):
            return __BINOP_V_V(x, y, i64, i64, timestamp, ADDI64, len, offset, out);
        case MTYPE2(TYPE_TIMESTAMP, TYPE_TIMESTAMP):
            return __BINOP_V_V(x, y, i64, i64, timestamp, ADDI64, len, offset, out);
        case MTYPE2(TYPE_TIMESTAMP, TYPE_F64):
            return __BINOP_V_A(x, y, i64, f64, timestamp, ADDF64, len, offset, out);
        case MTYPE2(TYPE_TIMESTAMP, -TYPE_F64):
            return __BINOP_V_A(x, y, i64, f64, timestamp, ADDF64, len, offset, out);
        case MTYPE2(TYPE_I64, TYPE_TIMESTAMP):
            return __BINOP_V_V(x, y, i64, i64, timestamp, ADDI64, len, offset, out);
        case MTYPE2(TYPE_F64, TYPE_TIMESTAMP):
            return __BINOP_V_V(x, y, f64, i64, timestamp, ADDF64, len, offset, out);
        case MTYPE2(-TYPE_I64, TYPE_TIMESTAMP):
            return __BINOP_A_V(x, y, i64, i64, timestamp, ADDI64, len, offset, out);
        default:
            THROW(ERR_TYPE, "add: unsupported types: '%s, '%s", type_name(x->type), type_name(y->type));
    }
}

obj_p ray_sub_partial(obj_p x, obj_p y, u64_t len, u64_t offset, obj_p out) {
    switch (MTYPE2(x->type, y->type)) {
        case MTYPE2(-TYPE_I32, -TYPE_I32):
            return i32(SUBI32(x->i32, y->i32));
        case MTYPE2(-TYPE_I32, -TYPE_I64):
            return i64(SUBI64(i32_to_i64(x->i32), y->i64));
        case MTYPE2(-TYPE_I32, -TYPE_F64):
            return f64(SUBF64(x->i32, y->f64));
        case MTYPE2(-TYPE_TIME, -TYPE_TIME):
            return i32(SUBI32(x->i32, y->i32));
        case MTYPE2(-TYPE_DATE, -TYPE_DATE):
            return i32(SUBI32(x->i32, y->i32));
        case MTYPE2(-TYPE_I64, -TYPE_I32):
            return i64(SUBI64(x->i64, i32_to_i64(y->i32)));
        case MTYPE2(-TYPE_I64, -TYPE_I64):
            return i64(SUBI64(x->i64, y->i64));
        case MTYPE2(-TYPE_I64, -TYPE_F64):
            return f64(SUBF64(x->i64, y->f64));
        case MTYPE2(-TYPE_F64, -TYPE_F64):
            return f64(SUBF64(x->f64, y->f64));
        case MTYPE2(-TYPE_F64, -TYPE_I64):
            return f64(SUBF64(x->f64, (f64_t)y->i64));
        case MTYPE2(-TYPE_TIMESTAMP, -TYPE_I64):
            return timestamp(SUBI64(x->i64, y->i64));
        case MTYPE2(-TYPE_TIMESTAMP, TYPE_TIMESTAMP):
            return i64(SUBI64(x->i64, y->i64));
        case MTYPE2(-TYPE_I64, TYPE_I64):
            return __BINOP_A_V(x, y, i64, i64, i64, SUBI64, len, offset, out);
        case MTYPE2(-TYPE_I64, TYPE_F64):
            return __BINOP_A_V(x, y, i64, f64, f64, SUBF64, len, offset, out);
        case MTYPE2(-TYPE_F64, TYPE_F64):
            return __BINOP_A_V(x, y, f64, f64, f64, SUBF64, len, offset, out);
        case MTYPE2(-TYPE_F64, TYPE_I64):
            return __BINOP_A_V(x, y, f64, i64, f64, SUBF64, len, offset, out);
        case MTYPE2(TYPE_I64, -TYPE_I64):
            return __BINOP_V_A(x, y, i64, i64, i64, SUBI64, len, offset, out);
        case MTYPE2(TYPE_I64, -TYPE_F64):
            return __BINOP_V_A(x, y, i64, f64, f64, SUBF64, len, offset, out);
        case MTYPE2(TYPE_I64, TYPE_I64):
            return __BINOP_V_V(x, y, i64, i64, i64, SUBI64, len, offset, out);
        case MTYPE2(TYPE_I64, TYPE_F64):
            return __BINOP_V_V(x, y, i64, f64, f64, SUBF64, len, offset, out);
        case MTYPE2(TYPE_F64, -TYPE_F64):
            return __BINOP_V_A(x, y, f64, f64, f64, SUBF64, len, offset, out);
        case MTYPE2(TYPE_F64, -TYPE_I64):
            return __BINOP_V_A(x, y, f64, i64, f64, SUBF64, len, offset, out);
        case MTYPE2(TYPE_F64, TYPE_F64):
            return __BINOP_V_V(x, y, f64, f64, f64, SUBF64, len, offset, out);
        case MTYPE2(TYPE_F64, TYPE_I64):
            return __BINOP_V_V(x, y, f64, i64, f64, SUBF64, len, offset, out);
        case MTYPE2(TYPE_TIMESTAMP, -TYPE_I64):
            return __BINOP_V_A(x, y, i64, i64, timestamp, SUBI64, len, offset, out);
        case MTYPE2(TYPE_TIMESTAMP, TYPE_I64):
            return __BINOP_V_V(x, y, i64, i64, timestamp, SUBI64, len, offset, out);
        case MTYPE2(TYPE_TIMESTAMP, TYPE_TIMESTAMP):
            return __BINOP_V_V(x, y, i64, i64, i64, SUBI64, len, offset, out);
        default:
            THROW(ERR_TYPE, "sub: unsupported types: '%s, '%s", type_name(x->type), type_name(y->type));
    }
}

obj_p ray_mul_partial(obj_p x, obj_p y, u64_t len, u64_t offset, obj_p out) {
    switch (MTYPE2(x->type, y->type)) {
        case MTYPE2(-TYPE_I32, -TYPE_I32):
            return i32(MULI32(x->i32, y->i32));
        case MTYPE2(-TYPE_I32, -TYPE_I64):
            return i64(MULI64(i32_to_i64(x->i32), y->i64));
        case MTYPE2(-TYPE_I32, -TYPE_F64):
            return f64(MULF64(x->i32, y->f64));
        case MTYPE2(-TYPE_I64, -TYPE_I32):
            return i64(MULI64(x->i64, i32_to_i64(y->i32)));
        case MTYPE2(-TYPE_I64, -TYPE_I64):
            return i64(MULI64(x->i64, y->i64));
        case MTYPE2(-TYPE_I64, -TYPE_F64):
            return f64(MULF64(x->i64, y->f64));
        case MTYPE2(-TYPE_F64, -TYPE_F64):
            return f64(MULF64(x->f64, y->f64));
        case MTYPE2(-TYPE_F64, -TYPE_I64):
            return f64(MULF64(x->f64, (f64_t)y->i64));
        case MTYPE2(-TYPE_I64, TYPE_I64):
            return __BINOP_A_V(x, y, i64, i64, i64, MULI64, len, offset, out);
        case MTYPE2(-TYPE_I64, TYPE_F64):
            return __BINOP_A_V(x, y, i64, f64, f64, MULF64, len, offset, out);
        case MTYPE2(-TYPE_F64, TYPE_F64):
            return __BINOP_A_V(x, y, f64, f64, f64, MULF64, len, offset, out);
        case MTYPE2(-TYPE_F64, TYPE_I64):
            return __BINOP_A_V(x, y, f64, i64, f64, MULF64, len, offset, out);
        case MTYPE2(TYPE_I64, -TYPE_I64):
            return __BINOP_V_A(x, y, i64, i64, i64, MULI64, len, offset, out);
        case MTYPE2(TYPE_I64, -TYPE_F64):
            return __BINOP_V_A(x, y, i64, f64, f64, MULF64, len, offset, out);
        case MTYPE2(TYPE_I64, TYPE_I64):
            return __BINOP_V_V(x, y, i64, i64, i64, MULI64, len, offset, out);
        case MTYPE2(TYPE_I64, TYPE_F64):
            return __BINOP_V_V(x, y, i64, f64, f64, MULF64, len, offset, out);
        case MTYPE2(TYPE_F64, -TYPE_F64):
            return __BINOP_V_A(x, y, f64, f64, f64, MULF64, len, offset, out);
        case MTYPE2(TYPE_F64, -TYPE_I64):
            return __BINOP_V_A(x, y, f64, i64, f64, MULF64, len, offset, out);
        case MTYPE2(TYPE_F64, TYPE_F64):
            return __BINOP_V_V(x, y, f64, f64, f64, MULF64, len, offset, out);
        case MTYPE2(TYPE_F64, TYPE_I64):
            return __BINOP_V_V(x, y, f64, i64, f64, MULF64, len, offset, out);
        default:
            THROW(ERR_TYPE, "mul: unsupported types: '%s, '%s", type_name(x->type), type_name(y->type));
    }
}

obj_p ray_div_partial(obj_p x, obj_p y, u64_t len, u64_t offset, obj_p out) {
    switch (MTYPE2(x->type, y->type)) {
        case MTYPE2(-TYPE_I32, -TYPE_I32):
            return i32(DIVI32(x->i32, y->i32));
        case MTYPE2(-TYPE_I32, -TYPE_I64):
            return i64(DIVI64(i32_to_i64(x->i32), y->i64));
        case MTYPE2(-TYPE_I32, -TYPE_F64):
            return f64(DIVF64(x->i32, y->f64));
        case MTYPE2(-TYPE_I64, -TYPE_I32):
            return i64(DIVI64(x->i64, i32_to_i64(y->i32)));
        case MTYPE2(-TYPE_I64, -TYPE_I64):
            return i64(DIVI64(x->i64, y->i64));
        case MTYPE2(-TYPE_I64, -TYPE_F64):
            return i64(DIVI64(x->i64, y->f64));
        case MTYPE2(-TYPE_F64, -TYPE_F64):
            return i64(DIVF64(x->f64, y->f64));
        case MTYPE2(-TYPE_F64, -TYPE_I64):
            return i64(DIVF64(x->f64, y->i64));
        case MTYPE2(-TYPE_I64, TYPE_I64):
            return __BINOP_A_V(x, y, i64, i64, i64, DIVI64, len, offset, out);
        case MTYPE2(-TYPE_I64, TYPE_F64):
            return __BINOP_A_V(x, y, i64, f64, i64, DIVI64, len, offset, out);
        case MTYPE2(-TYPE_F64, TYPE_F64):
            return __BINOP_A_V(x, y, f64, f64, i64, DIVF64, len, offset, out);
        case MTYPE2(-TYPE_F64, TYPE_I64):
            return __BINOP_A_V(x, y, f64, i64, i64, DIVF64, len, offset, out);
        case MTYPE2(TYPE_I64, -TYPE_I64):
            return __BINOP_V_A(x, y, i64, i64, i64, DIVI64, len, offset, out);
        case MTYPE2(TYPE_I64, -TYPE_F64):
            return __BINOP_V_A(x, y, i64, f64, i64, DIVI64, len, offset, out);
        case MTYPE2(TYPE_I64, TYPE_I64):
            return __BINOP_V_V(x, y, i64, i64, i64, DIVI64, len, offset, out);
        case MTYPE2(TYPE_I64, TYPE_F64):
            return __BINOP_V_V(x, y, i64, f64, i64, DIVI64, len, offset, out);
        case MTYPE2(TYPE_F64, -TYPE_F64):
            return __BINOP_V_A(x, y, f64, f64, i64, DIVF64, len, offset, out);
        case MTYPE2(TYPE_F64, -TYPE_I64):
            return __BINOP_V_A(x, y, f64, i64, i64, DIVF64, len, offset, out);
        case MTYPE2(TYPE_F64, TYPE_F64):
            return __BINOP_V_V(x, y, f64, f64, i64, DIVF64, len, offset, out);
        case MTYPE2(TYPE_F64, TYPE_I64):
            return __BINOP_V_V(x, y, f64, i64, i64, DIVF64, len, offset, out);
        default:
            THROW(ERR_TYPE, "div: unsupported types: '%s, '%s", type_name(x->type), type_name(y->type));
    }
}

obj_p ray_fdiv_partial(obj_p x, obj_p y, u64_t len, u64_t offset, obj_p out) {
    switch (MTYPE2(x->type, y->type)) {
        case MTYPE2(-TYPE_I32, -TYPE_I32):
            return f64(FDIVI32(x->i32, y->i32));
        case MTYPE2(-TYPE_I32, -TYPE_I64):
            return f64(FDIVI64(i32_to_i64(x->i32), y->i64));
        case MTYPE2(-TYPE_I32, -TYPE_F64):
            return f64(FDIVF64(x->i32, y->f64));
        case MTYPE2(-TYPE_I64, -TYPE_I32):
            return f64(FDIVI64(x->i64, i32_to_i64(y->i32)));
        case MTYPE2(-TYPE_I64, -TYPE_I64):
            return f64(FDIVI64(x->i64, y->i64));
        case MTYPE2(-TYPE_I64, -TYPE_F64):
            return f64(FDIVI64(x->i64, y->f64));
        case MTYPE2(-TYPE_F64, -TYPE_F64):
            return f64(FDIVF64(x->f64, y->f64));
        case MTYPE2(-TYPE_F64, -TYPE_I64):
            return f64(FDIVF64(x->f64, y->i64));
        case MTYPE2(-TYPE_I64, TYPE_I64):
            return __BINOP_A_V(x, y, i64, i64, f64, FDIVI64, len, offset, out);
        case MTYPE2(-TYPE_I64, TYPE_F64):
            return __BINOP_A_V(x, y, i64, f64, f64, FDIVI64, len, offset, out);
        case MTYPE2(-TYPE_F64, TYPE_F64):
            return __BINOP_A_V(x, y, f64, f64, f64, FDIVF64, len, offset, out);
        case MTYPE2(-TYPE_F64, TYPE_I64):
            return __BINOP_A_V(x, y, f64, i64, f64, FDIVF64, len, offset, out);
        case MTYPE2(TYPE_I64, -TYPE_I64):
            return __BINOP_V_A(x, y, i64, i64, f64, FDIVI64, len, offset, out);
        case MTYPE2(TYPE_I64, -TYPE_F64):
            return __BINOP_V_A(x, y, i64, f64, f64, FDIVI64, len, offset, out);
        case MTYPE2(TYPE_I64, TYPE_I64):
            return __BINOP_V_V(x, y, i64, i64, f64, FDIVI64, len, offset, out);
        case MTYPE2(TYPE_I64, TYPE_F64):
            return __BINOP_V_V(x, y, i64, f64, f64, FDIVI64, len, offset, out);
        case MTYPE2(TYPE_F64, -TYPE_F64):
            return __BINOP_V_A(x, y, f64, f64, f64, FDIVF64, len, offset, out);
        case MTYPE2(TYPE_F64, -TYPE_I64):
            return __BINOP_V_A(x, y, f64, i64, f64, FDIVF64, len, offset, out);
        case MTYPE2(TYPE_F64, TYPE_F64):
            return __BINOP_V_V(x, y, f64, f64, f64, FDIVF64, len, offset, out);
        case MTYPE2(TYPE_F64, TYPE_I64):
            return __BINOP_V_V(x, y, f64, i64, f64, FDIVF64, len, offset, out);
        default:
            THROW(ERR_TYPE, "fdiv: unsupported types: '%s, '%s", type_name(x->type), type_name(y->type));
    }
}

obj_p ray_mod_partial(obj_p x, obj_p y, u64_t len, u64_t offset, obj_p out) {
    switch (MTYPE2(x->type, y->type)) {
        case MTYPE2(-TYPE_I32, -TYPE_I32):
            return i32(MODI32(x->i32, y->i32));
        case MTYPE2(-TYPE_I32, -TYPE_I64):
            return i64(MODI64(i32_to_i64(x->i32), y->i64));
        case MTYPE2(-TYPE_I32, TYPE_I32):
            return __BINOP_A_V(x, y, i32, i32, i32, MODI32, len, offset, out);
        case MTYPE2(TYPE_I32, -TYPE_I32):
            return __BINOP_V_A(x, y, i32, i32, i32, MODI32, len, offset, out);
        case MTYPE2(TYPE_I32, TYPE_I32):
            return __BINOP_V_V(x, y, i32, i32, i32, MODI32, len, offset, out);
        case MTYPE2(-TYPE_I64, TYPE_I32):
            return __BINOP_A_V(x, y, i64, i32, i32, MODI32, len, offset, out);
        case MTYPE2(TYPE_I32, -TYPE_I64):
            return __BINOP_V_A(x, y, i32, i64, i32, MODI64, len, offset, out);
        case MTYPE2(-TYPE_I64, -TYPE_I32):
            return i64(MODI64(x->i64, i32_to_i64(y->i32)));
        case MTYPE2(-TYPE_I64, -TYPE_I64):
            return i64(MODI64(x->i64, y->i64));
        case MTYPE2(-TYPE_I64, TYPE_I64):
            return __BINOP_A_V(x, y, i64, i64, i64, MODI64, len, offset, out);
        case MTYPE2(TYPE_I64, -TYPE_I64):
            return __BINOP_V_A(x, y, i64, i64, i64, MODI64, len, offset, out);
        case MTYPE2(TYPE_I64, TYPE_I64):
            return __BINOP_V_V(x, y, i64, i64, i64, MODI64, len, offset, out);
        default:
            THROW(ERR_TYPE, "mod: unsupported types: '%s, '%s", type_name(x->type), type_name(y->type));
    }
}

obj_p ray_xbar_partial(obj_p x, obj_p y, u64_t len, u64_t offset, obj_p out) {
    switch (MTYPE2(x->type, y->type)) {
        case MTYPE2(-TYPE_I32, -TYPE_I32):
            return i32(XBARI32(x->i32, y->i32));
        case MTYPE2(-TYPE_I32, -TYPE_I64):
            return i64(XBARI64(i32_to_i64(x->i32), y->i64));
        case MTYPE2(-TYPE_I32, TYPE_I32):
            return __BINOP_A_V(x, y, i32, i32, i32, XBARI32, len, offset, out);
        case MTYPE2(TYPE_I32, -TYPE_I32):
            return __BINOP_V_A(x, y, i32, i32, i32, XBARI32, len, offset, out);
        case MTYPE2(TYPE_I32, TYPE_I32):
            return __BINOP_V_V(x, y, i32, i32, i32, XBARI32, len, offset, out);
        case MTYPE2(TYPE_I32, -TYPE_I64):
            return __BINOP_V_A(x, y, i32, i64, i32, XBARI64, len, offset, out);
        case MTYPE2(TYPE_I32, TYPE_I64):
            return __BINOP_V_V(x, y, i32, i64, i32, XBARI64, len, offset, out);
        case MTYPE2(-TYPE_I64, -TYPE_I32):
            return i64(XBARI64(x->i64, i32_to_i64(y->i32)));
        case MTYPE2(-TYPE_I64, -TYPE_I64):
            return i64(XBARI64(x->i64, y->i64));
        case MTYPE2(-TYPE_I64, TYPE_I64):
            return __BINOP_A_V(x, y, i64, i64, i64, XBARI64, len, offset, out);
        case MTYPE2(TYPE_I64, -TYPE_I64):
            return __BINOP_V_A(x, y, i64, i64, i64, XBARI64, len, offset, out);
        case MTYPE2(TYPE_I64, TYPE_I64):
            return __BINOP_V_V(x, y, i64, i64, i64, XBARI64, len, offset, out);
        default:
            THROW(ERR_TYPE, "xbar: unsupported types: '%s, '%s", type_name(x->type), type_name(y->type));
    }
}

obj_p ray_sum_partial(u64_t len, i64_t input[]) {
    u64_t i;
    i64_t isum;

    for (i = 0, isum = 0; i < len; i++)
        isum += (input[i] == NULL_I64) ? 0 : input[i];

    return i64(isum);
}

obj_p ray_sum(obj_p x) {
    u64_t i, l, chunks, chunk;
    i64_t isum, *xii;
    f64_t fsum, *xfi;
    pool_p pool;
    obj_p res;

    switch (x->type) {
        case -TYPE_I64:
            return clone_obj(x);
        case -TYPE_F64:
            return clone_obj(x);
        case TYPE_I64:
            l = x->len;
            xii = AS_I64(x);

            pool = pool_get();
            chunks = pool_split_by(pool, l, 0);

            if (chunks == 1) {
                for (i = 0, isum = 0; i < l; i++)
                    isum += (xii[i] == NULL_I64) ? 0 : xii[i];

                return i64(isum);
            }

            pool_prepare(pool);
            chunk = l / chunks;

            for (i = 0; i < chunks - 1; i++)
                pool_add_task(pool, (raw_p)ray_sum_partial, 2, chunk, xii + i * chunk);

            pool_add_task(pool, (raw_p)ray_sum_partial, 2, l - i * chunk, xii + i * chunk);

            res = pool_run(pool);

            for (i = 0, isum = 0; i < chunks; i++)
                isum += AS_LIST(res)[i]->i64;

            drop_obj(res);

            return i64(isum);

        case TYPE_F64:
            l = x->len;
            xfi = AS_F64(x);
            for (i = 0, fsum = 0; i < l; i++)
                fsum += xfi[i];

            return f64(fsum);
        case TYPE_MAPGROUP:
            return aggr_sum(AS_LIST(x)[0], AS_LIST(x)[1]);

        case TYPE_PARTEDI64:
            l = x->len;
            for (i = 0, isum = 0; i < l; i++) {
                res = ray_sum(AS_LIST(x)[i]);
                isum += (res->i64 == NULL_I64) ? 0 : res->i64;
                drop_obj(res);
            }

            return i64(isum);

        case TYPE_PARTEDF64:
            l = x->len;
            for (i = 0, fsum = 0; i < l; i++) {
                res = ray_sum(AS_LIST(x)[i]);
                fsum += res->f64;
                drop_obj(res);
            }

            return f64(fsum);

        default:
            THROW(ERR_TYPE, "sum: unsupported type: '%s", type_name(x->type));
    }
}

obj_p ray_avg(obj_p x) {
    u64_t i, l = 0, n;
    i64_t isum = 0, *xivals = NULL;
    f64_t fsum = 0.0, *xfvals = NULL;

    switch (x->type) {
        case -TYPE_I64:
        case -TYPE_F64:
            return clone_obj(x);
        case TYPE_I64:
            l = x->len;
            xivals = AS_I64(x);

            for (i = 0, n = 0; i < l; i++) {
                if (xivals[i] != NULL_I64)
                    isum += xivals[i];
                else
                    n++;
            }

            return f64((f64_t)isum / (l - n));

        case TYPE_F64:
            l = x->len;
            xfvals = AS_F64(x);

            for (i = 0, n = 0; i < l; i++)
                fsum += xfvals[i];

            return f64(fsum / l);

        case TYPE_MAPGROUP:
            return aggr_avg(AS_LIST(x)[0], AS_LIST(x)[1]);

        default:
            THROW(ERR_TYPE, "avg: unsupported type: '%s", type_name(x->type));
    }
}

obj_p ray_min(obj_p x) {
    u64_t i, l = 0;
    i64_t imin, iv, *xivals = NULL;
    f64_t fmin = 0.0, *xfvals = NULL;
    obj_p res;

    switch (x->type) {
        case TYPE_I64:
        case TYPE_TIMESTAMP:
            l = x->len;

            if (!l)
                return i64(NULL_I64);

            xivals = AS_I64(x);
            imin = xivals[0];

            xivals = AS_I64(x);
            imin = xivals[0];

            for (i = 0; i < l; i++) {
                iv = (xivals[i] == NULL_I64) ? MAX_I64 : xivals[i];
                imin = iv < imin ? iv : imin;
            }

            res = i64(imin);
            res->type = -x->type;

            return res;

        case TYPE_F64:
            l = x->len;

            if (!l)
                return f64(NULL_F64);

            xfvals = AS_F64(x);
            fmin = xfvals[0];

            for (i = 0; i < l; i++)
                fmin = xfvals[i] < fmin ? xfvals[i] : fmin;

            return f64(fmin);

        case TYPE_MAPGROUP:
            return aggr_min(AS_LIST(x)[0], AS_LIST(x)[1]);

        default:
            THROW(ERR_TYPE, "min: unsupported type: '%s", type_name(x->type));
    }
}

obj_p ray_max(obj_p x) {
    u64_t i, l = 0;
    i64_t imax = 0, *xivals = NULL;
    f64_t fmax = 0.0, *xfvals = NULL;
    obj_p res;

    switch (x->type) {
        case TYPE_I64:
        case TYPE_TIMESTAMP:
            l = x->len;

            if (!l)
                return i64(NULL_I64);

            xivals = AS_I64(x);
            imax = xivals[0];

            for (i = 0; i < l; i++)
                imax = xivals[i] > imax ? xivals[i] : imax;

            res = i64(imax);
            res->type = -x->type;

            return res;

        case TYPE_F64:
            l = x->len;

            if (!l)
                return f64(NULL_F64);

            xfvals = AS_F64(x);
            fmax = xfvals[0];

            for (i = 0; i < l; i++)
                fmax = xfvals[i] > fmax ? xfvals[i] : fmax;

            return f64(fmax);

        case TYPE_MAPGROUP:
            return aggr_max(AS_LIST(x)[0], AS_LIST(x)[1]);

        default:
            THROW(ERR_TYPE, "max: unsupported type: '%s", type_name(x->type));
    }
}

obj_p ray_dev(obj_p x) {
    u64_t i, l = 0;
    f64_t fsum = 0.0, favg = 0.0, *xfvals = NULL;
    obj_p res;

    switch (x->type) {
        case TYPE_I64:
        case TYPE_TIMESTAMP:
            l = x->len;

            if (!l)
                return f64(NULL_F64);

            xfvals = AS_F64(x);
            fsum = xfvals[0];

            for (i = 0; i < l; i++)
                fsum += xfvals[i];

            favg = fsum / l;

            fsum = 0.0;
            for (i = 0; i < l; i++)
                fsum += pow(xfvals[i] - favg, 2);

            res = f64(sqrt(fsum / l));

            return res;

        case TYPE_F64:
            l = x->len;

            if (!l)
                return f64(NULL_F64);

            xfvals = AS_F64(x);
            fsum = xfvals[0];

            for (i = 0; i < l; i++)
                fsum += xfvals[i];

            favg = fsum / l;

            fsum = 0.0;
            for (i = 0; i < l; i++)
                fsum += pow(xfvals[i] - favg, 2);

            res = f64(sqrt(fsum / l));

            return res;

        case TYPE_MAPGROUP:
            return aggr_dev(AS_LIST(x)[0], AS_LIST(x)[1]);

        default:
            THROW(ERR_TYPE, "dev: unsupported type: '%s", type_name(x->type));
    }
}

obj_p ray_med(obj_p x) {
    u64_t l;
    i64_t *xisort;
    f64_t *xfsort, med;
    obj_p sort;

    switch (x->type) {
        case TYPE_I64:
            l = x->len;

            if (l == 0)
                return null(TYPE_F64);

            sort = ray_asc(x);
            xisort = AS_I64(sort);
            med = (l % 2 == 0) ? (xisort[l / 2 - 1] + xisort[l / 2]) / 2.0 : xisort[l / 2];
            drop_obj(sort);

            return f64(med);

        case TYPE_F64:
            l = x->len;

            if (l == 0)
                return null(TYPE_F64);

            sort = ray_asc(x);
            xfsort = AS_F64(sort);
            med = (l % 2 == 0) ? (xfsort[l / 2 - 1] + xfsort[l / 2]) / 2.0 : xfsort[l / 2];
            drop_obj(sort);

            return f64(med);

        case TYPE_MAPGROUP:
            return aggr_med(AS_LIST(x)[0], AS_LIST(x)[1]);

        default:
            THROW(ERR_TYPE, "med: unsupported type: '%s", type_name(x->type));
    }
}

obj_p ray_round(obj_p x) {
    u64_t i, l = 0;
    i64_t *rvals;
    f64_t *xfvals;
    obj_p res;

    switch (x->type) {
        case -TYPE_F64:
            return i64(ROUNDF64(x->f64));
        case TYPE_F64:
            l = x->len;

            res = F64(l);
            rvals = AS_I64(res);
            xfvals = AS_F64(x);

            for (i = 0; i < l; i++)
                rvals[i] = ROUNDF64(xfvals[i]);

            return res;
        default:
            THROW(ERR_TYPE, "round: unsupported type: '%s", type_name(x->type));
    }
}

obj_p ray_floor(obj_p x) {
    u64_t i, l = 0;
    i64_t *rvals;
    f64_t *xfvals;
    obj_p res;

    switch (x->type) {
        case -TYPE_F64:
            return i64(FLOORF64(x->f64));
        case TYPE_F64:
            l = x->len;

            res = F64(l);
            rvals = AS_I64(res);
            xfvals = AS_F64(x);

            for (i = 0; i < l; i++)
                rvals[i] = FLOORF64(xfvals[i]);

            return res;
        default:
            THROW(ERR_TYPE, "floor: unsupported type: '%s", type_name(x->type));
    }
}

obj_p ray_ceil(obj_p x) {
    u64_t i, l = 0;
    i64_t *rvals;
    f64_t *xfvals;
    obj_p res;

    switch (x->type) {
        case -TYPE_F64:
            return i64(CEILF64(x->f64));
        case TYPE_F64:
            l = x->len;

            res = F64(l);
            rvals = AS_I64(res);
            xfvals = AS_F64(x);

            for (i = 0; i < l; i++)
                rvals[i] = CEILF64(xfvals[i]);

            return res;
        default:
            THROW(ERR_TYPE, "ceil: unsupported type: '%s", type_name(x->type));
    }
}

obj_p binop_map(raw_p op, obj_p x, obj_p y) {
    pool_p pool = runtime_get()->pool;
    u64_t i, l, n, chunk;
    obj_p v, res;
    raw_p argv[5];
    i8_t t;

    if (IS_VECTOR(x) && IS_VECTOR(y)) {
        if (x->len != y->len)
            THROW(ERR_LENGTH, "vectors must have the same length");

        l = x->len;
    } else if (IS_VECTOR(x))
        l = x->len;
    else if (IS_VECTOR(y))
        l = y->len;
    else {
        argv[0] = (raw_p)x;
        argv[1] = (raw_p)y;
        return pool_call_task_fn(op, 2, argv);
    }

    t = (op == ray_fdiv_partial) ? TYPE_F64 : infer_math_type(x, y);

    n = pool_split_by(pool, l, 0);
    res = vector(t, l);

    if (n == 1) {
        argv[0] = (raw_p)x;
        argv[1] = (raw_p)y;
        argv[2] = (raw_p)l;
        argv[3] = (raw_p)0;
        argv[4] = (raw_p)res;
        v = pool_call_task_fn(op, 5, argv);
        if (IS_ERROR(v)) {
            res->len = 0;
            drop_obj(res);
            return v;
        }
        return res;
    }

    pool_prepare(pool);
    chunk = l / n;

    for (i = 0; i < n - 1; i++)
        pool_add_task(pool, op, 5, x, y, chunk, i * chunk, res);

    pool_add_task(pool, op, 5, x, y, l - i * chunk, i * chunk, res);

    v = pool_run(pool);
    if (IS_ERROR(v)) {
        res->len = 0;
        drop_obj(res);
        return v;
    }

    drop_obj(v);

    return res;
}

obj_p ray_add(obj_p x, obj_p y) { return binop_map(ray_add_partial, x, y); }
obj_p ray_sub(obj_p x, obj_p y) { return binop_map(ray_sub_partial, x, y); }
obj_p ray_mul(obj_p x, obj_p y) { return binop_map(ray_mul_partial, x, y); }
obj_p ray_div(obj_p x, obj_p y) { return binop_map(ray_div_partial, x, y); }
obj_p ray_fdiv(obj_p x, obj_p y) { return binop_map(ray_fdiv_partial, x, y); }
obj_p ray_mod(obj_p x, obj_p y) { return binop_map(ray_mod_partial, x, y); }
obj_p ray_xbar(obj_p x, obj_p y) { return binop_map(ray_xbar_partial, x, y); }
