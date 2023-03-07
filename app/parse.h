#ifndef PARSE_H
#define PARSE_H

#include "../core/storm.h"

typedef struct parser_t
{
    str_t filename;
    str_t input;
    str_t current;
    i64_t line;
    i64_t column;
} __attribute__((aligned(16))) parser_t;

extern value_t parse(str_t filename, str_t input);

#endif
