/*
 *   Copyright (c) 2024 Anton Kundenko <singaraiona@gmail.com>
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

#include "time.h"
#include "util.h"

RAYASSERT(sizeof(struct timestruct_t) == 16, time_h)

timestruct_t time_from_i32(i32_t offset) {
    if (offset == NULL_I32)
        return (timestruct_t){.null = 1};

    return (timestruct_t){
        .null = 0,
        .sign = offset < 0 ? 1 : 0,
        .hours = (u8_t)(offset / 10000),
        .mins = (u8_t)((offset % 10000) / 100),
        .secs = (u8_t)(offset % 100),
        .msecs = 0,
    };
}
timestruct_t time_from_str(str_p src, u64_t len) {}
i64_t time_into_i32(timestruct_t dt) {}
obj_p ray_time(obj_p arg) {}