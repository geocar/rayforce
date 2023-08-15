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

#ifndef LAMBDA_H
#define LAMBDA_H

#include "rayforce.h"
#include "nfo.h"

#define as_lambda(obj) ((lambda_t *)(as_string(obj)))

typedef struct lambda_t
{
    obj_t args;       // vector of arguments names
    obj_t locals;     // list of dicts of local variables
    obj_t body;       // body of lambda
    obj_t code;       // string of bytecode
    obj_t constants;  // list of lambda constants
    nfo_t nfo;        // nfo from cc phase
    i32_t stack_size; // size of stack frame
} lambda_t;

obj_t lambda(obj_t args, obj_t body, obj_t code, nfo_t nfo);

#endif
