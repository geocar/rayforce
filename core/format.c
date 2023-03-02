#include "format.h"
#include "storm.h"
#include <stdio.h>
#include <string.h>
#include "alloc.h"

#define MAX_I64_WIDTH 20
#define MAX_ROW_WIDTH MAX_I64_WIDTH * 2

Result nil_fmt(str *buffer)
{
    *buffer = (str)a0_alloc(4);
    strncpy(*buffer, "nil", 4);
    return Ok;
}

Result scalar_i64_fmt(str *buffer, g0 value)
{
    *buffer = (str)a0_alloc(MAX_I64_WIDTH);
    if (snprintf(*buffer, MAX_I64_WIDTH, "%lld", value->i64_value) < 0)
    {
        return FormatError;
    }
    return Ok;
}

Result vector_i64_fmt(str *buffer, g0 value)
{
    str buf;
    i64 count, remains, len;

    *buffer = (str)a0_alloc(MAX_ROW_WIDTH + 4);
    buf = *buffer;
    strncpy(buf, "[", 2);
    buf += 1;

    count = value->list_value.len < 1 ? 0 : value->list_value.len - 1;
    remains = MAX_ROW_WIDTH;
    len = 0;

    for (i64 i = 0; i < count; i++)
    {
        remains = MAX_ROW_WIDTH - (buf - (*buffer));
        len = snprintf(buf, remains, "%lld, ", ((i64 *)value->list_value.ptr)[i]);
        if (len < 0)
        {
            return FormatError;
        }
        if (remains < len)
        {
            buf = (*buffer) + MAX_ROW_WIDTH - 1;
            break;
        }
        buf += len;
    }

    remains = MAX_ROW_WIDTH - (buf - (*buffer));
    if (value->list_value.len > 0 && remains > 0)
    {
        len = snprintf(buf, remains, "%lld", ((i64 *)value->list_value.ptr)[count]);
        if (len < 0)
        {
            return FormatError;
        }
        if (remains < len)
        {
            buf = (*buffer) + MAX_ROW_WIDTH - 1;
        }
        else
        {
            buf += len;
        }
    }

    if (remains < len)
    {
        strncpy(buf, "..", 3);
        buf += 2;
    }

    strncpy(buf, "]", 2);
    return Ok;
}

extern Result g0_fmt(str *buffer, g0 value)
{
    switch (value->type)
    {
    case -TYPE_I64:
    {
        return scalar_i64_fmt(buffer, value);
    }
    case TYPE_I64:
    {
        return vector_i64_fmt(buffer, value);
    }
    default:
    {
        return InvalidType;
    }
    }
}

extern void result_fmt(str *buffer, Result result)
{
    switch (result)
    {
    case Ok:
    {
        *buffer = (str)a0_alloc(3);
        strncpy(*buffer, "Ok", 3);
        break;
    }
    case FormatError:
    {
        *buffer = (str)a0_alloc(12);
        strncpy(*buffer, "FormatError", 12);
        break;
    }
    case InvalidType:
    {
        *buffer = (str)a0_alloc(12);
        strncpy(*buffer, "InvalidType", 12);
        break;
    }
    default:
    {
        *buffer = (str)a0_alloc(10);
        strncpy(*buffer, "Unknown", 10);
        break;
    }
    }
}
