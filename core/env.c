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

#include <stdarg.h>
#include "env.h"
#include "dict.h"
#include "nilary.h"
#include "unary.h"
#include "binary.h"
#include "ternary.h"
#include "nary.h"
#include "guid.h"
#include "runtime.h"

#define REC_SIZE (MAX_ARITY + 2)

#define REC(r, a, n, o)                          \
    {                                            \
        i64_t id = intern_keyword(n, strlen(n)); \
        env_record_t rec = {                     \
            .id = id,                            \
            .op = (i64_t)o,                      \
            .arity = a,                          \
        };                                       \
        push(&as_list(r)[a], env_record_t, rec); \
    }

// clang-format off
null_t init_functions(rf_object_t *records)
{
    // Nilary
    REC(records, 0, "halt",        OP_HALT);
    REC(records, 0, "env",         rf_env);
    REC(records, 0, "memstat",     rf_memstat);
            
    // Unary
    REC(records, 1, "type",        rf_type);
    REC(records, 1, "til" ,        rf_til);
    REC(records, 1, "trace",       OP_TRACE);
    REC(records, 1, "distinct",    rf_distinct);
    REC(records, 1, "group",       rf_group);
    REC(records, 1, "sum",         rf_sum);
    REC(records, 1, "avg",         rf_avg);
    REC(records, 1, "min",         rf_min);
    REC(records, 1, "max",         rf_max);
    REC(records, 1, "count",       rf_count);
    REC(records, 1, "not",         rf_not);
    REC(records, 1, "iasc",        rf_iasc);
    REC(records, 1, "idesc",       rf_idesc);
    REC(records, 1, "asc",         rf_asc);
    REC(records, 1, "desc",        rf_desc);
    REC(records, 1, "flatten",     list_flatten);
    REC(records, 1, "guid",        rf_guid_generate);
      
    // // Binary         
    // REC(records, 2, "==",       -TYPE_BOOL,       rf_eq_i64_i64,           {-TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "==",        TYPE_BOOL,       rf_eq_I64_i64,           { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "==",        TYPE_BOOL,       rf_eq_I64_I64,           { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, "==",       -TYPE_BOOL,       rf_eq_f64_f64,           {-TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "==",        TYPE_BOOL,       rf_eq_F64_f64,           { TYPE_F64,       -TYPE_F64         })
    // REC(records, 2, "==",        TYPE_BOOL,       rf_eq_F64_F64,           { TYPE_F64,        TYPE_F64         });
    // REC(records, 2, "==",       -TYPE_BOOL,       rf_eq_symbol_symbol,     {-TYPE_SYMBOL,    -TYPE_SYMBOL      });
    // REC(records, 2, "==",        TYPE_BOOL,       rf_eq_Symbol_symbol,     { TYPE_SYMBOL,    -TYPE_SYMBOL      });
    // REC(records, 2, "==",        TYPE_BOOL,       rf_eq_Symbol_Symbol,     { TYPE_SYMBOL,     TYPE_SYMBOL      });
    // REC(records, 2, "<",        -TYPE_BOOL,       rf_lt_i64_i64,           {-TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "<",        -TYPE_BOOL,       rf_lt_f64_f64,           {-TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "<",         TYPE_BOOL,       rf_lt_I64_i64,           { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "<",         TYPE_BOOL,       rf_lt_I64_I64,           { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, ">",        -TYPE_BOOL,       rf_gt_i64_i64,           {-TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, ">",        -TYPE_BOOL,       rf_gt_f64_f64,           {-TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, ">",         TYPE_BOOL,       rf_gt_I64_i64,           { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, ">",         TYPE_BOOL,       rf_gt_I64_I64,           { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, "<=",       -TYPE_BOOL,       rf_le_i64_i64,           {-TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "<=",       -TYPE_BOOL,       rf_le_f64_f64,           {-TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "<=",        TYPE_BOOL,       rf_le_I64_i64,           { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "<=",        TYPE_BOOL,       rf_le_I64_I64,           { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, ">=",       -TYPE_BOOL,       rf_ge_i64_i64,           {-TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, ">=",       -TYPE_BOOL,       rf_ge_f64_f64,           {-TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, ">=",        TYPE_BOOL,       rf_ge_I64_i64,           { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, ">=",        TYPE_BOOL,       rf_ge_I64_I64,           { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, "!=",        TYPE_BOOL,       rf_ne_i64_i64,           {-TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "!=",        TYPE_BOOL,       rf_ne_f64_f64,           {-TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "!=",       -TYPE_BOOL,       rf_ne_I64_i64,           { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "!=",       -TYPE_BOOL,       rf_ne_I64_I64,           { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, "and",       TYPE_BOOL,       rf_and_Bool_Bool,        { TYPE_BOOL,       TYPE_BOOL        });
    // REC(records, 2, "or",        TYPE_BOOL,       rf_or_Bool_Bool,         { TYPE_BOOL,       TYPE_BOOL        });
    // REC(records, 2, "+",        -TYPE_I64,        rf_add_i64_i64,          {-TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "+",        -TYPE_F64,        rf_add_f64_f64,          {-TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "+",         TYPE_I64,        rf_add_I64_i64,          { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "+",         TYPE_I64,        rf_add_I64_I64,          { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, "+",         TYPE_F64,        rf_add_F64_f64,          { TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "+",         TYPE_F64,        rf_add_F64_F64,          { TYPE_F64,        TYPE_F64         });
    // REC(records, 2, "-",        -TYPE_I64,        rf_sub_i64_i64,          {-TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "-",        -TYPE_F64,        rf_sub_f64_f64,          {-TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "-",         TYPE_I64,        rf_sub_I64_i64,          { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "-",         TYPE_I64,        rf_sub_I64_I64,          { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, "-",         TYPE_F64,        rf_sub_F64_f64,          { TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "-",         TYPE_F64,        rf_sub_F64_F64,          { TYPE_F64,        TYPE_F64         });
    // REC(records, 2, "*",        -TYPE_I64,        rf_mul_i64_i64,          {-TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "*",        -TYPE_F64,        rf_mul_f64_f64,          {-TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "*",         TYPE_I64,        rf_mul_I64_i64,          { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "*",         TYPE_I64,        rf_mul_I64_I64,          { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, "*",         TYPE_F64,        rf_mul_F64_f64,          { TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "*",         TYPE_F64,        rf_mul_F64_F64,          { TYPE_F64,        TYPE_F64         });
    // REC(records, 2, "/",        -TYPE_I64,        rf_div_i64_i64,          {-TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "/",         TYPE_I64,        rf_div_I64_i64,          { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "/",         TYPE_I64,        rf_div_I64_I64,          { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, "/",        -TYPE_F64,        rf_div_f64_f64,          {-TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "/",         TYPE_F64,        rf_div_F64_f64,          { TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "/",         TYPE_F64,        rf_div_F64_F64,          { TYPE_F64,        TYPE_F64         });
    // REC(records, 2, "%",        -TYPE_I64,        rf_mod_i64_i64,          {-TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "%",         TYPE_I64,        rf_mod_I64_i64,          { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "%",         TYPE_I64,        rf_mod_I64_I64,          { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, "%",        -TYPE_F64,        rf_mod_f64_f64,          {-TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "%",         TYPE_F64,        rf_mod_F64_f64,          { TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "%",         TYPE_F64,        rf_mod_F64_F64,          { TYPE_F64,        TYPE_F64         });
    // REC(records, 2, "div",      -TYPE_F64,        rf_fdiv_i64_i64,         {-TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "div",       TYPE_F64,        rf_fdiv_I64_i64,         { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "div",       TYPE_F64,        rf_fdiv_I64_I64,         { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, "div",      -TYPE_F64,        rf_fdiv_f64_f64,         {-TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "div",       TYPE_F64,        rf_fdiv_F64_f64,         { TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "div",       TYPE_F64,        rf_fdiv_F64_F64,         { TYPE_F64,        TYPE_F64         });
    // REC(records, 2, "like",     -TYPE_BOOL,       rf_like_Char_Char,       { TYPE_CHAR,       TYPE_CHAR        });
    // REC(records, 2, "dict",      TYPE_DICT,       rf_dict,                 { TYPE_ANY,        TYPE_ANY         });
    // REC(records, 2, "table",     TYPE_TABLE,      rf_table,                { TYPE_ANY,        TYPE_ANY         });
     
    // REC(records, 2, "get",      -TYPE_BOOL,       rf_get_Bool_i64,         { TYPE_BOOL,      -TYPE_I64         });
    // REC(records, 2, "get",       TYPE_BOOL,       rf_get_Bool_I64,         { TYPE_BOOL,       TYPE_I64         });
    // REC(records, 2, "get",      -TYPE_TIMESTAMP,  rf_get_Timestamp_i64,    { TYPE_TIMESTAMP, -TYPE_I64         });
    // REC(records, 2, "get",       TYPE_TIMESTAMP,  rf_get_Timestamp_I64,    { TYPE_TIMESTAMP,  TYPE_I64         });
    // REC(records, 2, "get",      -TYPE_GUID,       rf_get_Guid_i64,         { TYPE_GUID,      -TYPE_I64         });
    // REC(records, 2, "get",       TYPE_GUID,       rf_get_Guid_I64,         { TYPE_GUID,       TYPE_I64         });
    // REC(records, 2, "get",      -TYPE_I64,        rf_get_I64_i64,          { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "get",      -TYPE_F64,        rf_get_F64_i64,          { TYPE_F64,       -TYPE_I64         });
    // REC(records, 2, "get",      -TYPE_I64,        rf_get_I64_I64,          { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, "get",      -TYPE_F64,        rf_get_F64_I64,          { TYPE_F64,        TYPE_I64         });
    // REC(records, 2, "get",       TYPE_CHAR,       rf_get_Char_i64,         { TYPE_CHAR,      -TYPE_I64         });
    // REC(records, 2, "get",       TYPE_CHAR,       rf_get_Char_I64,         { TYPE_CHAR,       TYPE_I64         });
    // REC(records, 2, "get",       TYPE_ANY,        rf_get_List_i64,         { TYPE_LIST,      -TYPE_I64         });
    // REC(records, 2, "get",       TYPE_LIST,       rf_get_List_I64,         { TYPE_LIST,       TYPE_I64         });
    // REC(records, 2, "get",       TYPE_ANY,        rf_get_Table_symbol,     { TYPE_TABLE,     -TYPE_SYMBOL      });
    // REC(records, 2, "find",     -TYPE_I64,        rf_find_I64_i64,         { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "find",     -TYPE_I64,        rf_find_F64_f64,         { TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "find",      TYPE_I64,        rf_find_I64_I64,         { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, "find",      TYPE_I64,        rf_find_F64_F64,         { TYPE_F64,        TYPE_F64         });
    // REC(records, 2, "concat",    TYPE_BOOL,       rf_concat_bool_bool,     {-TYPE_BOOL,      -TYPE_BOOL        });
    // REC(records, 2, "concat",    TYPE_BOOL,       rf_concat_Bool_bool,     { TYPE_BOOL,      -TYPE_BOOL        });
    // REC(records, 2, "concat",    TYPE_BOOL,       rf_concat_Bool_Bool,     { TYPE_BOOL,       TYPE_BOOL        });
    // REC(records, 2, "concat",    TYPE_BOOL,       rf_concat_bool_Bool,     {-TYPE_BOOL,       TYPE_BOOL        });
    // REC(records, 2, "concat",    TYPE_I64,        rf_concat_i64_i64,       {-TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "concat",    TYPE_I64,        rf_concat_I64_i64,       { TYPE_I64,       -TYPE_I64         });
    // REC(records, 2, "concat",    TYPE_I64,        rf_concat_I64_I64,       { TYPE_I64,        TYPE_I64         });
    // REC(records, 2, "concat",    TYPE_I64,        rf_concat_i64_I64,       {-TYPE_I64,        TYPE_I64         });
    // REC(records, 2, "concat",    TYPE_F64,        rf_concat_f64_f64,       {-TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "concat",    TYPE_F64,        rf_concat_F64_f64,       { TYPE_F64,       -TYPE_F64         });
    // REC(records, 2, "concat",    TYPE_F64,        rf_concat_F64_F64,       { TYPE_F64,        TYPE_F64         });
    // REC(records, 2, "concat",    TYPE_F64,        rf_concat_f64_F64,       {-TYPE_F64,        TYPE_F64         });
    // REC(records, 2, "concat",    TYPE_CHAR,       rf_concat_char_char,     {-TYPE_CHAR,      -TYPE_CHAR        });
    // REC(records, 2, "concat",    TYPE_CHAR,       rf_concat_Char_char,     { TYPE_CHAR,      -TYPE_CHAR        });
    // REC(records, 2, "concat",    TYPE_CHAR,       rf_concat_Char_Char,     { TYPE_CHAR,       TYPE_CHAR        });
    // REC(records, 2, "concat",    TYPE_CHAR,       rf_concat_char_Char,     {-TYPE_CHAR,       TYPE_CHAR        });
    // REC(records, 2, "concat",    TYPE_LIST,       rf_concat_List_List,     { TYPE_LIST,       TYPE_LIST        });
    // REC(records, 2, "filter",    TYPE_I64,        rf_filter_I64_Bool,      { TYPE_I64,        TYPE_BOOL        });
    // REC(records, 2, "filter",    TYPE_TABLE,      rf_filter_Table_Bool,    { TYPE_TABLE,      TYPE_BOOL        });
    // REC(records, 2, "take",      TYPE_I64,        rf_take_i64_i64,         {-TYPE_I64,       -TYPE_I64         });
     
    // // Ternary       
    REC(records, 3, "rand",      rf_rand);
         
    // // Quaternary       
    // // Nary       
    REC(records, 5, "list",         rf_list);
    // REC(records, 5, "format",    TYPE_CHAR,       rf_format,               { 0                                 });
    // REC(records, 5, "print",     TYPE_NULL,       rf_print,                { 0                                 });
    REC(records, 5, "println",      rf_println);
}    
    
null_t init_typenames(i64_t *typenames)    
{
    typenames[-TYPE_BOOL      + TYPE_OFFSET] = symbol("bool").i64;
    typenames[-TYPE_I64       + TYPE_OFFSET] = symbol("i64").i64;
    typenames[-TYPE_F64       + TYPE_OFFSET] = symbol("f64").i64;
    typenames[-TYPE_SYMBOL    + TYPE_OFFSET] = symbol("symbol").i64;
    typenames[-TYPE_TIMESTAMP + TYPE_OFFSET] = symbol("timestamp").i64;
    typenames[-TYPE_GUID      + TYPE_OFFSET] = symbol("guid").i64;
    typenames[-TYPE_CHAR      + TYPE_OFFSET] = symbol("char").i64;
    typenames[ TYPE_NULL      + TYPE_OFFSET] = symbol("Null").i64;
    typenames[ TYPE_BOOL      + TYPE_OFFSET] = symbol("Bool").i64;
    typenames[ TYPE_I64       + TYPE_OFFSET] = symbol("I64").i64;
    typenames[ TYPE_F64       + TYPE_OFFSET] = symbol("F64").i64;
    typenames[ TYPE_SYMBOL    + TYPE_OFFSET] = symbol("Symbol").i64;
    typenames[ TYPE_TIMESTAMP + TYPE_OFFSET] = symbol("Timestamp").i64;
    typenames[ TYPE_GUID      + TYPE_OFFSET] = symbol("Guid").i64;
    typenames[ TYPE_CHAR      + TYPE_OFFSET] = symbol("Char").i64;
    typenames[ TYPE_LIST      + TYPE_OFFSET] = symbol("List").i64;
    typenames[ TYPE_DICT      + TYPE_OFFSET] = symbol("Dict").i64;
    typenames[ TYPE_TABLE     + TYPE_OFFSET] = symbol("Table").i64;
    typenames[ TYPE_FUNCTION  + TYPE_OFFSET] = symbol("Function").i64;
}


null_t init_kw_symbols()
{
    assert(intern_symbol("",      0)  == NULL_SYM);
    assert(intern_keyword("time", 4)  == KW_TIME);
    assert(intern_keyword("`",    1)  == KW_QUOTE);
    assert(intern_keyword("set",  3)  == KW_SET);
    assert(intern_keyword("let",  3)  == KW_LET);
    assert(intern_keyword("fn",   2)  == KW_FN);
}
// clang-format on

env_t create_env()
{
    rf_object_t functions = list(REC_SIZE);
    rf_object_t variables = dict(vector_symbol(0), list(0));

    for (i32_t i = 0; i < REC_SIZE; i++)
        as_list(&functions)[i] = vector(TYPE_CHAR, 0);

    init_kw_symbols();

    init_functions(&functions);

    env_t env = {
        .functions = functions,
        .variables = variables,
    };

    init_typenames(env.typenames);

    return env;
}

null_t free_env(env_t *env)
{
    rf_object_free(&env->variables);
    rf_object_free(&env->functions);
}

i64_t env_get_typename_by_type(env_t *env, type_t type)
{
    return env->typenames[type + TYPE_OFFSET];
}

type_t env_get_type_by_typename(env_t *env, i64_t name)
{
    for (i32_t i = 0; i < MAX_TYPE; i++)
        if (env->typenames[i] == name)
            return i - TYPE_OFFSET;

    return TYPE_NONE;
}

str_t env_get_typename(type_t type)
{
    env_t *env = &runtime_get()->env;
    i64_t name = env_get_typename_by_type(env, type);

    return symbols_get(name);
}