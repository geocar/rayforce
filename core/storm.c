#include "storm.h"
#include "format.h"
#include <stdio.h>
#include "alloc.h"

extern i8_t null(value_t *value)
{
    return value->type == TYPE_S0 && value->s0.ptr == NULL;
}

extern value_t error(i8_t code, str_t message)
{
    value_t error = {
        .type = TYPE_ERR,
        .error = {
            .code = code,
            .message = message,
        },
    };

    return error;
}

extern value_t i64(i64_t value)
{
    value_t scalar = {
        .type = -TYPE_I64,
        .i64 = value,
    };

    return scalar;
}

extern value_t f64(f64_t value)
{
    value_t scalar = {
        .type = -TYPE_F64,
        .f64 = value,
    };

    return scalar;
}

extern value_t vi64(i64_t *ptr, i64_t len)
{
    value_t vector = {
        .type = TYPE_I64,
        .s0 = {
            .ptr = ptr,
            .len = len,
        },
    };

    return vector;
}

extern value_t vf64(f64_t *ptr, i64_t len)
{
    value_t vector = {
        .type = TYPE_F64,
        .s0 = {
            .ptr = ptr,
            .len = len,
        },
    };

    return vector;
}

extern value_t s0(value_t *ptr, i64_t len)
{
    value_t list = {
        .type = TYPE_S0,
        .s0 = {
            .ptr = ptr,
            .len = len,
        },
    };

    return list;
}

extern nil_t value_free(value_t *value)
{
    switch (value->type)
    {
    case TYPE_I64:
    {
        storm_free(value->s0.ptr);
        break;
    }
    default:
    {
        printf("** Free: Invalid type\n");
        break;
    }
    }
}
