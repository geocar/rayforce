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
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include "parse.h"
#include "rayforce.h"
#include "heap.h"
#include "format.h"
#include "string.h"
#include "util.h"
#include "nfo.h"
#include "runtime.h"
#include "ops.h"
#include "eval.h"
#include "timestamp.h"
#include "error.h"

#define COMMANDS_LIST "\
  \\?  - Displays help.\n\
  \\t  - Measures the execution time of an expression.\n\
  \\\\  - Exits the application."

span_t span_start(parser_t *parser)
{
    span_t s = {
        .start_line = parser->line,
        .end_line = parser->line,
        .start_column = parser->column,
        .end_column = parser->column,
    };

    return s;
}

nil_t span_extend(parser_t *parser, span_t *span)
{
    span->end_line = parser->line;
    span->end_column = parser->column == 0 ? 0 : parser->column - 1;

    return;
}

obj_t parse_error(parser_t *parser, i64_t id, str_t msg)
{
    span_t span;
    obj_t obj = error_str(ERR_PARSE, msg);

    if (parser->nfo)
    {
        span = nfo_get(parser->nfo, id);
        as_error(obj)->locs = vn_list(1,
                                      vn_list(4,
                                              i64(span.id),                   // span
                                              clone(as_list(parser->nfo)[0]), // file
                                              null(0),                        // function
                                              clone(as_list(parser->nfo)[1])  // source
                                              ));
    }

    heap_free(msg);

    return obj;
}

bool_t is_whitespace(char_t c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool_t is_digit(char_t c)
{
    return c >= '0' && c <= '9';
}

bool_t is_alpha(char_t c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool_t is_alphanum(char_t c)
{
    return is_alpha(c) || is_digit(c);
}

bool_t is_op(char_t c)
{
    return strchr("+-*/%&|^~<>!=._", c) != NULL;
}

bool_t at_eof(char_t c)
{
    return c == '\0' || (signed char)c == EOF;
}

bool_t at_term(char_t c)
{
    return c == ')' || c == ']' || c == '}' || c == ':' || c == '\n';
}

bool_t is_at(obj_t token, char_t c)
{
    return token && token->type == -TYPE_CHAR && token->vchar == c;
}

bool_t is_at_term(obj_t token)
{
    return token && token->type == -TYPE_CHAR && at_term(token->vchar);
}

i8_t shift(parser_t *parser, i32_t num)
{
    i8_t res;

    if (at_eof(*parser->current))
        return 0;

    res = *parser->current;
    parser->current += num;
    parser->column += num;

    return res;
}

obj_t to_token(parser_t *parser)
{
    obj_t tok = vchar(*parser->current);
    tok->type = -TYPE_CHAR;
    nfo_insert(parser->nfo, (i64_t)tok, span_start(parser));

    return tok;
}

obj_t parse_timestamp(parser_t *parser)
{
    str_t end, current = parser->current;
    u32_t nanos;
    timestamp_t ts = {0};
    obj_t res;
    span_t span = span_start(parser);

    // check if null
    if (*current == '0')
    {
        if (*(current + 1) == 't')
        {
            shift(parser, 2);
            res = timestamp(NULL_I64);
            nfo_insert(parser->nfo, (i64_t)res, span);

            return res;
        }
    }

    // parse year
    if (is_digit(*current) &&
        is_digit(*(current + 1)) &&
        is_digit(*(current + 2)) &&
        is_digit(*(current + 3)))
    {
        ts.year = (*current - '0') * 1000 +
                  (*(current + 1) - '0') * 100 +
                  (*(current + 2) - '0') * 10 +
                  (*(current + 3) - '0');
        current += 4;
    }
    else
        return null(0);

    // skip dot
    if (*current != '.')
        return null(0);

    current++;

    // parse month
    if (is_digit(*current) &&
        is_digit(*(current + 1)))
    {
        ts.month = (*current - '0') * 10 +
                   (*(current + 1) - '0');

        current += 2;

        if (ts.month > 12)
        {
            span.start_column = current - parser->current - 2;
            span.end_column += current - parser->current - 1;
            nfo_insert(parser->nfo, parser->count, span);
            return parse_error(parser, parser->count++, str_fmt(0, "Month is out of range"));
        }
    }
    else
        return null(0);

    // skip dot
    if (*current != '.')
        return null(0);

    current++;

    // parse day
    if (is_digit(*current) &&
        is_digit(*(current + 1)))
    {
        ts.day = (*current - '0') * 10 +
                 (*(current + 1) - '0');

        current += 2;

        if (ts.day > 31)
        {
            span.start_column = current - parser->current - 2;
            span.end_column += current - parser->current - 1;
            nfo_insert(parser->nfo, parser->count, span);
            return parse_error(parser, parser->count++, str_fmt(0, "Day is out of range"));
        }
    }
    else
        return null(0);

    // just date passed
    if (*current != 'D')
    {
        shift(parser, current - parser->current);
        res = timestamp(timestamp_into_i64(ts));

        span_extend(parser, &span);
        nfo_insert(parser->nfo, (i64_t)res, span);

        return res;
    }

    current++;

    // parse hour
    if (is_digit(*current) &&
        is_digit(*(current + 1)))
    {
        ts.hours = (*current - '0') * 10 +
                   (*(current + 1) - '0');

        current += 2;

        if (ts.hours > 23)
        {
            span.start_column = current - parser->current - 2;
            span.end_column += current - parser->current - 1;
            nfo_insert(parser->nfo, parser->count, span);
            return parse_error(parser, parser->count++, str_fmt(0, "Hour is out of range"));
        }
    }
    else
        return null(0);

    // skip colon
    if (*current != ':')
        return null(0);

    current++;

    // parse minute
    if (is_digit(*current) &&
        is_digit(*(current + 1)))
    {
        ts.mins = (*current - '0') * 10 +
                  (*(current + 1) - '0');

        current += 2;

        if (ts.mins > 59)
        {
            span.start_column = current - parser->current - 2;
            span.end_column += current - parser->current - 1;
            nfo_insert(parser->nfo, parser->count, span);
            return parse_error(parser, parser->count++, str_fmt(0, "Minute is out of range"));
        }
    }
    else
        return null(0);

    // skip colon
    if (*current != ':')
        return null(0);

    current++;

    // parse second
    if (is_digit(*current) &&
        is_digit(*(current + 1)))
    {
        ts.secs = (*current - '0') * 10 +
                  (*(current + 1) - '0');

        current += 2;

        if (ts.secs > 59)
        {
            span.start_column = current - parser->current - 2;
            span.end_column += current - parser->current - 1;
            nfo_insert(parser->nfo, parser->count, span);
            return parse_error(parser, parser->count++, str_fmt(0, "Second is out of range"));
        }
    }
    else
        return null(0);

    // skip dot
    if (*current != '.')
        return null(0);

    current++;

    // parse nanos
    nanos = strtoul(current, &end, 10);

    if (end == current)
        return null(0);

    ts.nanos = nanos;
    shift(parser, end - parser->current);
    res = timestamp(timestamp_into_i64(ts));

    span_extend(parser, &span);
    nfo_insert(parser->nfo, (i64_t)res, span);

    return res;
}

obj_t parse_number(parser_t *parser)
{
    str_t end;
    i64_t num_i64;
    f64_t num_f64;
    obj_t num;
    span_t span = span_start(parser);

    // check if null
    if (*parser->current == '0')
    {
        if (*(parser->current + 1) == 'i')
        {
            shift(parser, 2);
            num = i64(NULL_I64);
            nfo_insert(parser->nfo, (i64_t)num, span);

            return num;
        }

        if (*(parser->current + 1) == 'f')
        {
            shift(parser, 2);
            num = f64(NULL_F64);
            nfo_insert(parser->nfo, (i64_t)num, span);

            return num;
        }

        if (*(parser->current + 1) == 't')
        {
            shift(parser, 2);
            num = timestamp(NULL_I64);
            nfo_insert(parser->nfo, (i64_t)num, span);

            return num;
        }

        if (*(parser->current + 1) == 'g')
        {
            shift(parser, 2);
            num = guid(NULL_GUID);
            nfo_insert(parser->nfo, (i64_t)num, span);

            return num;
        }
    }

    errno = 0;

    num_i64 = strtoll(parser->current, &end, 10);

    if ((num_i64 == LONG_MAX || num_i64 == LONG_MIN) && errno == ERANGE)
    {
        span.end_column += (end - parser->current - 1);
        nfo_insert(parser->nfo, parser->count, span);
        return parse_error(parser, parser->count++, str_fmt(0, "Number is out of range"));
    }

    if (end == parser->current)
        return error_str(ERR_PARSE, "Invalid number");

    // try double instead
    if (*end == '.')
    {
        num_f64 = strtod(parser->current, &end);

        if (errno == ERANGE)
        {
            span.end_column += (end - parser->current);
            nfo_insert(parser->nfo, parser->count, span);
            return parse_error(parser, parser->count++, str_fmt(0, "Number is out of range"));
        }

        num = f64(num_f64);
    }
    else
        num = i64(num_i64);

    shift(parser, end - parser->current);
    span_extend(parser, &span);
    nfo_insert(parser->nfo, (i64_t)num, span);

    return num;
}

obj_t parse_char(parser_t *parser)
{
    span_t span = span_start(parser);
    str_t pos = parser->current + 1; // skip '''
    obj_t res = NULL;
    i64_t id;
    char_t ch;

    if (at_eof(*pos) || *pos == '\n')
    {
        shift(parser, pos - parser->current);
        span_extend(parser, &span);
        nfo_insert(parser->nfo, (i64_t)res, span);

        res = null(TYPE_SYMBOL);
        res->attrs = ATTR_QUOTED;

        return res;
    }

    ch = *pos++;

    if (*pos != '\'')
    {
        // continue parsing a symbol
        while (!at_eof(*pos) && *pos != '\n' && (is_alphanum(*pos) || is_op(*pos)))
            pos++;

        if (*pos == '\'')
        {
            span.end_column += (pos - parser->current);
            nfo_insert(parser->nfo, parser->count, span);
            return parse_error(parser, parser->count++, str_fmt(0, "Invalid literal: char can not contain more than one symbol"));
        }

        id = intern_symbol(parser->current + 1, pos - (parser->current + 1));
        res = i64(id);
        res->type = -TYPE_SYMBOL;
        res->attrs = ATTR_QUOTED;
        shift(parser, pos - parser->current);
        span_extend(parser, &span);
        nfo_insert(parser->nfo, (i64_t)res, span);

        return res;
    }

    res = vchar(ch);

    shift(parser, 3);
    span_extend(parser, &span);
    nfo_insert(parser->nfo, (i64_t)res, span);

    return res;
}

obj_t parse_string(parser_t *parser)
{
    span_t span = span_start(parser);
    str_t pos = parser->current + 1; // skip '"'
    i32_t len = 0;
    obj_t str, err;
    char_t nl = '\0', lf = '\n', cr = '\r', tb = '\t';

    str = string(0);

    while (!at_eof(*pos) && *pos != '\n')
    {
        if (*pos == '\\')
        {
            switch (*++pos)
            {
            case '"':
                push_raw(&str, pos++);
                break;
            case 'n':
                push_raw(&str, &lf);
                pos++;
                break;
            case 'r':
                push_raw(&str, &cr);
                pos++;
                break;
            case 't':
                push_raw(&str, &tb);
                pos++;
                break;
            default:
                drop(str);
                span.end_column += (pos - parser->current);
                nfo_insert(parser->nfo, parser->count, span);
                err = parse_error(parser, parser->count++, str_fmt(0, "Invalid escape sequence"));
            }

            continue;
        }
        else if (*pos == '"')
            break;

        push_raw(&str, pos++);
    }

    if ((*pos++) != '"')
    {
        drop(str);
        span.end_column += (pos - parser->current);
        nfo_insert(parser->nfo, parser->count, span);
        err = parse_error(parser, parser->count++, str_fmt(0, "Expected '\"'"));

        return err;
    }

    len = pos - parser->current - 2;

    shift(parser, len + 2);
    span_extend(parser, &span);
    nfo_insert(parser->nfo, (i64_t)str, span);

    push_raw(&str, &nl);

    return str;
}

obj_t parse_symbol(parser_t *parser)
{
    str_t pos = parser->current;
    obj_t res = NULL;
    span_t span = span_start(parser);
    i64_t id;

    if (strncmp(parser->current, "true", 4) == 0)
    {
        shift(parser, 4);
        span_extend(parser, &span);
        res = bool(true);
        nfo_insert(parser->nfo, (i64_t)res, span);

        return res;
    }

    if (strncmp(parser->current, "false", 5) == 0)
    {
        shift(parser, 5);
        span_extend(parser, &span);
        res = bool(false);
        nfo_insert(parser->nfo, (i64_t)res, span);

        return res;
    }

    if (strncmp(parser->current, "null", 4) == 0)
    {
        shift(parser, 4);
        span_extend(parser, &span);
        res = null(0);
        nfo_insert(parser->nfo, (i64_t)res, span);

        return res;
    }

    // Skip first char and proceed until the end of the symbol
    do
    {
        pos++;
    } while (*pos && (is_alphanum(*pos) || is_op(*pos)));

    id = intern_symbol(parser->current, pos - parser->current);

    if (parser->replace_symbols)
        res = env_get_internal_function_by_id(id);

    if (!res)
        res = symboli64(id);

    shift(parser, pos - parser->current);
    span_extend(parser, &span);
    nfo_insert(parser->nfo, (i64_t)res, span);

    return res;
}

obj_t parse_vector(parser_t *parser)
{
    obj_t tok, vec = vector_i64(0), err;
    i32_t i;
    span_t span = span_start(parser);

    shift(parser, 1); // skip '['
    parser->replace_symbols = false;
    tok = advance(parser);
    parser->replace_symbols = true;

    while (!is_at(tok, ']'))
    {
        if (is_error(tok))
        {
            drop(vec);
            return tok;
        }

        if (is_at(tok, '\0') || is_at_term(tok))
        {
            err = parse_error(parser, (i64_t)tok, str_fmt(0, "Expected ']'"));
            drop(vec);
            drop(tok);

            return err;
        }

        if (tok->type == -TYPE_BOOL)
        {
            if (vec->len > 0 && vec->type != TYPE_BOOL)
            {
                err = parse_error(parser, (i64_t)tok, str_fmt(0, "Invalid token in vector"));
                drop(vec);
                drop(tok);

                return err;
            }

            vec->type = TYPE_BOOL;
            push_raw(&vec, &tok->bool);
        }
        else if (tok->type == -TYPE_I64)
        {
            if (vec->type == TYPE_I64)
                push_raw(&vec, &tok->i64);
            else if (vec->type == TYPE_F64)
            {
                push_raw(&vec, &tok->i64);
            }
            else
            {
                err = parse_error(parser, (i64_t)tok, str_fmt(0, "Invalid token in vector"));
                drop(vec);
                drop(tok);

                return err;
            }
        }
        else if (tok->type == -TYPE_F64)
        {
            if (vec->type == TYPE_F64)
                push_raw(&vec, &tok->f64);
            else if (vec->type == TYPE_I64)
            {
                vec->type = TYPE_F64;
                for (i = 0; i < (i32_t)vec->len; i++)
                    as_f64(vec)[i] = (f64_t)as_i64(vec)[i];

                push_raw(&vec, &tok->f64);
            }
            else
            {
                err = parse_error(parser, (i64_t)tok, str_fmt(0, "Invalid token in vector"));
                drop(vec);
                drop(tok);

                return err;
            }
        }
        else if (tok->type == -TYPE_SYMBOL)
        {
            if (vec->type == TYPE_SYMBOL || (vec->len == 0))
            {
                vec->type = TYPE_SYMBOL;
                push_raw(&vec, &tok->i64);
            }
            else
            {
                err = parse_error(parser, (i64_t)tok, str_fmt(0, "Invalid token in vector"));
                drop(vec);
                drop(tok);

                return err;
            }
        }
        else if (tok->type == -TYPE_TIMESTAMP)
        {
            if (vec->type == TYPE_TIMESTAMP || (vec->len == 0))
            {
                push_raw(&vec, &tok->i64);
                vec->type = TYPE_TIMESTAMP;
            }
            else
            {
                err = parse_error(parser, (i64_t)tok, str_fmt(0, "Invalid token in vector"));
                drop(vec);
                drop(tok);

                return err;
            }
        }
        else
        {
            err = parse_error(parser, (i64_t)tok, str_fmt(0, "Invalid token in vector"));
            drop(vec);
            drop(tok);

            return err;
        }

        drop(tok);
        span_extend(parser, &span);
        parser->replace_symbols = false;
        tok = advance(parser);
        parser->replace_symbols = true;
    }

    drop(tok);
    span_extend(parser, &span);
    nfo_insert(parser->nfo, (i64_t)vec, span);

    return vec;
}

obj_t parse_list(parser_t *parser)
{
    obj_t lst = NULL, tok, args, body, err;
    span_t span = span_start(parser);

    shift(parser, 1); // skip '('
    tok = advance(parser);

    // parse lambda
    if (tok->type == -TYPE_SYMBOL && tok->i64 == KW_FN)
    {
        drop(tok);

        args = advance(parser);
        if (is_error(args))
            return args;

        if (args->type != TYPE_SYMBOL)
        {
            if (args->type != TYPE_I64 || args->len != 0)
            {
                err = parse_error(parser, (i64_t)args, str_fmt(0, "fn: expected type 'Symbol as arguments."));
                drop(args);
                return err;
            }

            // empty args
            args->type = TYPE_SYMBOL;
        }

        body = parse_do(parser);
        if (is_error(body))
        {
            drop(args);
            return body;
        }

        tok = advance(parser);

        if (!is_at(tok, ')'))
        {
            span_extend(parser, &span);
            nfo_insert(parser->nfo, parser->count, span);
            err = parse_error(parser, parser->count++, str_fmt(0, "fn: expected ')'"));
            drop(args);
            drop(body);
            drop(tok);

            return err;
        }

        span_extend(parser, &span);
        lst = lambda(args, body, clone(parser->nfo));
        nfo_insert(parser->nfo, (i64_t)lst, span);
        // insert self into nfo
        nfo_insert(as_lambda(lst)->nfo, (i64_t)lst, span);
        drop(tok);

        return lst;
    }

    while (!is_at(tok, ')'))
    {
        if (is_error(tok))
        {
            drop(lst);
            return tok;
        }

        if (at_eof(*parser->current))
        {
            nfo_insert(parser->nfo, parser->count, span);
            err = parse_error(parser, parser->count++, str_fmt(0, "Expected ')'"));
            drop(lst);
            drop(tok);

            return err;
        }

        if (is_at_term(tok))
        {
            err = parse_error(parser, (i64_t)tok, str_fmt(0, "There is no opening found for: '%c'", tok->vchar));
            drop(lst);
            drop(tok);

            return err;
        }

        if (!lst)
            lst = vn_list(1, tok);
        else
            push_obj(&lst, tok);

        span_extend(parser, &span);
        tok = advance(parser);
    }

    span_extend(parser, &span);
    nfo_insert(parser->nfo, (i64_t)lst, span);
    drop(tok);

    return lst;
}

obj_t parse_dict(parser_t *parser)
{
    obj_t tok, keys = NULL, vals = list(0), d, err;
    span_t span = span_start(parser);

    shift(parser, 1); // skip '{'
    parser->replace_symbols = false;
    tok = advance(parser);
    parser->replace_symbols = true;

    while (!is_at(tok, '}'))
    {
        if (is_error(tok))
        {
            drop(keys);
            drop(vals);
            return tok;
        }

        if (at_eof(*parser->current) || is_at_term(tok))
        {
            nfo_insert(parser->nfo, parser->count, span);
            err = parse_error(parser, parser->count++, str_fmt(0, "Expected '}'"));
            drop(keys);
            drop(vals);
            drop(tok);

            return err;
        }

        if (!keys)
            keys = vector(tok->type, 0);

        push_obj(&keys, tok);

        span_extend(parser, &span);
        tok = advance(parser);

        if (is_error(tok))
        {
            drop(keys);
            drop(vals);

            return tok;
        }

        if (!is_at(tok, ':'))
        {
            err = parse_error(parser, (i64_t)tok, str_fmt(0, "Expected ':'"));
            drop(vals);
            drop(keys);
            drop(tok);

            return err;
        }

        span_extend(parser, &span);
        drop(tok);
        tok = advance(parser);

        if (is_error(tok))
        {
            drop(keys);
            drop(vals);

            return tok;
        }

        if (is_at_term(tok))
        {
            nfo_insert(parser->nfo, parser->count, span);
            err = parse_error(parser, parser->count++, str_fmt(0, "Expected value folowing ':'"));
            drop(keys);
            drop(vals);
            drop(tok);

            return err;
        }

        push_obj(&vals, tok);

        span_extend(parser, &span);
        parser->replace_symbols = false;
        tok = advance(parser);
        parser->replace_symbols = true;
    }

    drop(tok);

    d = dict(keys, vals);
    span_extend(parser, &span);
    nfo_insert(parser->nfo, (i64_t)d, span);

    return d;
}

obj_t parse_command(parser_t *parser)
{
    obj_t v, err;
    span_t span = span_start(parser);

    shift(parser, 1); // skip '\'

    if ((*parser->current) == '?')
    {
        shift(parser, 1);
        printf("%s** Commands list:\n%s%s\n", YELLOW, COMMANDS_LIST, RESET);
        return null(0);
    }
    if ((*parser->current) == 't')
    {
        shift(parser, 1);
        v = parse_do(parser);
        if (is_error(v))
            return v;

        return vn_list(2, env_get_internal_function("time"), v);
    }
    if ((*parser->current) == '\\')
    {
        shift(parser, 1);
        return vn_list(1, env_get_internal_function("exit"));
    }

    nfo_insert(parser->nfo, parser->count, span);
    err = parse_error(parser, parser->count++, str_fmt(0, "Invalid command. Type '\\?' for commands list."));
    return err;
}

nil_t skip_whitespaces(parser_t *parser)
{
    while (true)
    {
        if (at_eof(*parser->current))
            break;

        // Skip shebang
        if (*parser->current == '#' && *(parser->current + 1) == '!')
        {
            while (*parser->current != '\n' && !at_eof(*parser->current))
                parser->current++;

            parser->line++;
            parser->column = 0;
        }

        // Handle whitespace characters
        if (is_whitespace(*parser->current))
        {
            if (*parser->current == '\n')
            {
                parser->line++;
                parser->column = 0;
            }
            else
                parser->column++;

            parser->current++;
        }

        // Handle comments
        else if (*parser->current == ';')
        {
            while (*parser->current != '\n' && !at_eof(*parser->current))
            {
                parser->current++;
                parser->column++;
            }
        }

        else
            break;
    }
}

obj_t advance(parser_t *parser)
{
    obj_t tok = NULL, err = NULL;

    skip_whitespaces(parser);

    if (at_eof(*parser->current))
        return to_token(parser);

    if ((*parser->current) == '[')
        return parse_vector(parser);

    if ((*parser->current) == '(')
        return parse_list(parser);

    if ((*parser->current) == '{')
        return parse_dict(parser);

    if (is_digit(*parser->current))
    {
        tok = parse_timestamp(parser);
        if (!is_null(tok))
            return tok;

        drop(tok);
    }

    if (((*parser->current) == '-' && is_digit(*(parser->current + 1))) || is_digit(*parser->current))
        return parse_number(parser);

    if (is_alpha(*parser->current) || is_op(*parser->current))
        return parse_symbol(parser);

    if ((*parser->current) == '\'')
        return parse_char(parser);

    if ((*parser->current) == '"')
        return parse_string(parser);

    if ((*parser->current) == '\\')
        return parse_command(parser);

    if (at_term(*parser->current))
    {
        tok = to_token(parser);
        shift(parser, 1);

        return tok;
    }

    nfo_insert(parser->nfo, (i64_t)tok, span_start(parser));
    err = parse_error(parser, (i64_t)tok, str_fmt(0, "Unexpected token: '%c'", *parser->current));
    drop(tok);

    return err;
}

obj_t parse_do(parser_t *parser)
{
    obj_t tok, car = NULL, lst = NULL;

    while (!at_eof(*parser->current))
    {
        tok = advance(parser);

        if (is_error(tok))
        {
            drop(car);
            drop(lst);
            return tok;
        }

        if (is_at(tok, '\0'))
        {
            drop(tok);
            break;
        }

        if (is_at_term(tok))
        {
            drop(tok);
            // roll back one token
            parser->current--;
            parser->column--;
            break;
        }

        if (car == NULL)
            car = tok;
        else if (lst == NULL)
            lst = vn_list(3, env_get_internal_function("do"), car, tok);
        else
            push_obj(&lst, tok);
    }

    return lst ? lst : car;
}

obj_t parse(str_t input, obj_t nfo)
{
    obj_t res, err;
    span_t span;
    parser_t parser = {
        .input = input,
        .current = input,
        .line = 0,
        .column = 0,
        .nfo = nfo,
        .replace_symbols = true,
    };

    res = parse_do(&parser);

    if (is_error(res))
        return res;

    if (!at_eof(*parser.current))
    {
        span = nfo_get(parser.nfo, (i64_t)res);
        span.start_column = span.end_column + 1;
        span.end_column = span.start_column;
        nfo_insert(parser.nfo, parser.count, span);
        err = parse_error(&parser, parser.count++, str_fmt(0, "Unparsed input remains"));
        drop(res);
        return err;
    }

    return res;
}
