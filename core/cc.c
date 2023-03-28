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

#include <stdio.h>
#include "cc.h"
#include "vm.h"
#include "alloc.h"
#include "format.h"
#include "util.h"
#include "vector.h"

#define push_opcode(x)       \
    vector_reserve(code, 1); \
    as_string(code)[offset++] = x;

#define push_object(x)                            \
    vector_reserve(code, sizeof(rf_object_t));    \
    *(rf_object_t *)(code->adt.ptr + offset) = x; \
    offset += sizeof(rf_object_t);

typedef struct dispatch_record_t
{
    str_t name;
    i8_t args[8];
    i8_t ret;
    vm_opcode_t opcode;
} dispatch_record_t;

#define DISPATCH_RECORD_SIZE 3
#define DISPATCH_TABLE_SIZE 5

// clang-format off
static dispatch_record_t _DISPATCH_TABLE[DISPATCH_TABLE_SIZE][DISPATCH_RECORD_SIZE] = {
    // Nilary
    {
        {"halt", {0},  TYPE_LIST, OP_HALT}
    },
    // Unary
    {
        {"-",    {-TYPE_I64}, -TYPE_I64,  OP_HALT},            
    },
    // Binary
    {
        {"+", {-TYPE_I64, -TYPE_I64}, -TYPE_I64, OP_ADDI}, 
        {"+", {-TYPE_F64, -TYPE_F64}, -TYPE_F64, OP_ADDF},
    },
    // Ternary
    {{0}},
    // Quaternary
    {{0}},
};
// clang-format on

i8_t cc_compile_code(rf_object_t *object, rf_object_t *code, u32_t len)
{
    u32_t offset = 0, arity, i = 0, j = 0, match = 0;
    rf_object_t *car, err;
    dispatch_record_t *rec;

    switch (object->type)
    {
    case TYPE_LIST:
        if (object->adt.len == 0)
        {
            push_object(object_clone(object));
            break;
        }

        car = &as_list(object)[0];
        if (car->type != -TYPE_SYMBOL)
        {
            object_free(code);
            err = error(ERR_LENGTH, "compile list: expected symbol in a head");
            err.id = car->id;
            *code = err;
            return TYPE_ERROR;
        }

        arity = object->adt.len - 1;
        if (arity > 4)
        {
            object_free(code);
            err = error(ERR_LENGTH, "compile list: too many arguments");
            err.id = object->id;
            *code = err;
            return TYPE_ERROR;
        }

        while ((rec = &_DISPATCH_TABLE[arity][i++]))
        {
            if (i > DISPATCH_RECORD_SIZE)
                break;

            if (rec->name != 0 && strcmp(symbols_get(car->i64), rec->name) == 0)
            {
                for (j = 0; j < arity; j++)
                {
                    if (as_list(object)[j + 1].type != rec->args[j])
                        break;

                    match++;
                }

                if (match < arity)
                {
                    match = 0;
                    continue;
                }

                for (j = 1; j < object->adt.len; j++)
                {
                    push_opcode(OP_PUSH);
                    push_object(object_clone(&as_list(object)[j]));
                }

                push_opcode(rec->opcode);

                break;
            }
        }

        if (!match)
        {
            push_opcode(OP_PUSH);
            err = error(ERR_LENGTH, "compile list: function proto or arity mismatch");
            err.id = object->id;
            push_object(err);
        }
    }

    // push_opcode(OP_HALT);

    return code;
}

rf_object_t cc_compile(rf_object_t *object)
{
    u32_t len = 2 * sizeof(rf_object_t);
    rf_object_t code = string(len);
    cc_compile_code(object, &code, len);
    // debug("CODE: %s\n", cc_code_fmt(as_string(&code)));
    return code;
}

str_t cc_code_fmt(str_t code)
{
    i32_t p = 0, c = 0;
    str_t ip = code;
    str_t s = str_fmt(0, "-- code:\n");

    p = strlen(s);

    while (*ip != OP_HALT)
    {
        switch (*ip++)
        {
        case OP_PUSH:
            p += str_fmt_into(0, p, &s, "%.4d: push %p\n", c++, ((rf_object_t *)(ip + 1)));
            ip += sizeof(rf_object_t);
            break;
        case OP_POP:
            p += str_fmt_into(0, p, &s, "%.4d: pop\n", c++);
            break;
        case OP_ADDI:
            p += str_fmt_into(0, p, &s, "%.4d: addi\n", c++);
            break;
        case OP_ADDF:
            p += str_fmt_into(0, p, &s, "%.4d: addf\n", c++);
            break;
        default:
            p += str_fmt_into(0, p, &s, "%.4d: unknown %d\n", c++, *ip);
            break;
        }
    }

    str_fmt_into(0, p, &s, "%.4d: halt", c++);

    return s;
}