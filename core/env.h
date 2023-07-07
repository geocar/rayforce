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

#ifndef ENV_H
#define ENV_H

#include "rayforce.h"
#include "alloc.h"
#include "vm.h"
#include "vector.h"

// offset in array of typenames for each type
#define TYPE_OFFSET TYPE_CHAR
#define MAX_TYPE (TYPE_ERROR + TYPE_OFFSET + 2)

// reserved symbols (keywords)
#define NULL_SYM 0
#define KW_TIME -1
#define KW_QUOTE -2
#define KW_SET -3
#define KW_LET -4
#define KW_FN -5
#define KW_SELF -6
#define KW_IF -7
#define KW_TRY -8
#define KW_CATCH -9
#define KW_THROW -10
#define KW_MAP -11
#define KW_SELECT -12
#define KW_FROM -13
#define KW_WHERE -14
#define KW_BY -15
#define KW_ORDER -16

// primitives prototypes
typedef rf_object_t (*unary_t)(rf_object_t *);
typedef rf_object_t (*binary_t)(rf_object_t *, rf_object_t *);
typedef rf_object_t (*vary_t)(rf_object_t *, i64_t n);

/*
 *  Environment
 */
typedef struct env_t
{
    rf_object_t functions;     // dict, containing function primitives
    rf_object_t variables;     // dict, containing mappings variables names to their values
    i64_t typenames[MAX_TYPE]; // array of symbols contains typenames, maps type id to type name
} env_t;

env_t create_env();
null_t free_env(env_t *env);

i64_t env_get_typename_by_type(env_t *env, type_t type);
str_t env_get_typename(type_t type);
type_t env_get_type_by_typename(env_t *env, i64_t name);
rf_object_t env_set(env_t *env, rf_object_t *key, rf_object_t val);
rf_object_t env_get(env_t *env, rf_object_t *key);

rf_object_t rf_env();
rf_object_t rf_memstat();

#endif
