#include <stdio.h>
#include "parse.h"
#include "../core/storm.h"
#include "../core/alloc.h"
#include "../core/format.h"
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

typedef struct
{
    u8_t is_int;
    union
    {
        i64_t i64;
        f64_t f64;
    };
} num_t;

parser_t new_parser()
{
    parser_t parser = {
        .filename = NULL,
        .input = NULL,
        .current = NULL,
        .line = 0,
        .column = 0,
    };

    return parser;
}

u8_t is_whitespace(u8_t c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

u8_t is_digit(u8_t c)
{
    return c >= '0' && c <= '9';
}

u8_t is_alpha(u8_t c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

u8_t is_alphanum(u8_t c)
{
    return is_alpha(c) || is_digit(c);
}

u8_t at_eof(u8_t c)
{
    return c == '\n';
}

num_t parse_number(str_t *current)
{
    num_t num;
    str_t end;

    errno = 0;

    num.i64 = strtoll(*current, &end, 10);

    if ((num.i64 == LONG_MAX || num.i64 == LONG_MIN) && errno == ERANGE)
    {
        num.is_int = 1;
        return num;
    }

    if (end == *current)
        return num;

    // try double instead
    if (*end == '.')
    {
        num.f64 = strtod(*current, &end);

        if (errno == ERANGE)
            return num;
    }

    *current = end;

    return num;
}

value_t next(parser_t *parser)
{
    str_t *current = &parser->current;

    if (at_eof(**current))
        return s0(NULL, 0);

    while (is_whitespace(**current))
        (*current)++;

    // if ((**current) == '[')
    // {
    //     i64_t *vec_i64 = NULL;
    //     f64_t *vec_f64 = NULL;

    //     (*current)++;
    // }

    if ((**current) == '-' || is_digit(**current))
    {
        str_t end;
        i64_t num;
        value_t res;
        f64_t dnum;

        errno = 0;

        num = strtoll(*current, &end, 10);

        if ((num == LONG_MAX || num == LONG_MIN) && errno == ERANGE)
            return error(ERR_PARSE, "Number out of range");

        if (end == *current)
            return error(ERR_PARSE, "Invalid number");

        // try double instead
        if (*end == '.')
        {
            dnum = strtod(*current, &end);

            if (errno == ERANGE)
                return error(ERR_PARSE, "Number out of range");

            res = f64(dnum);
        }
        else
        {

            res = i64(num);
        }

        *current = end;

        return res;
    }

    return s0(NULL, 0);
}

value_t parse_program(parser_t *parser)
{
    str_t err_msg;
    value_t token;

    // do
    // {
    token = next(parser);
    // } while (token != NULL);

    if (!at_eof(*parser->current))
        return error(ERR_PARSE, str_fmt("Unexpected token: %s", parser->current));

    return token;
}

extern value_t parse(str_t filename, str_t input)
{
    value_t result_t;

    parser_t parser = {
        .filename = filename,
        .input = input,
        .current = input,
        .line = 0,
        .column = 0,
    };

    result_t = parse_program(&parser);

    return result_t;
}
