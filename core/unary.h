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

#ifndef MONAD_H
#define MONAD_H

#include "rayforce.h"
#include "ops.h"

obj_t rf_call_unary(u8_t attrs, unary_f f, obj_t x);
obj_t rf_get(obj_t x);
obj_t rf_type(obj_t x);
obj_t rf_count(obj_t x);
obj_t rf_til(obj_t x);
obj_t rf_distinct(obj_t x);
obj_t rf_group(obj_t x);
obj_t rf_group_remap(obj_t x);
obj_t rf_sum(obj_t x);
obj_t rf_avg(obj_t x);
obj_t rf_min(obj_t x);
obj_t rf_max(obj_t x);
obj_t rf_not(obj_t x);
obj_t rf_iasc(obj_t x);
obj_t rf_idesc(obj_t x);
obj_t rf_asc(obj_t x);
obj_t rf_desc(obj_t x);
obj_t rf_guid_generate(obj_t x);
obj_t rf_neg(obj_t x);
obj_t rf_where(obj_t x);
obj_t rf_key(obj_t x);
obj_t rf_value(obj_t x);
obj_t rf_read(obj_t x);
obj_t rf_parse(obj_t x);
obj_t rf_read_parse_compile(obj_t x);

#endif
