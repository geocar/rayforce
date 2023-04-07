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
#include <string.h>
#include "vm.h"
#include "rayforce.h"
#include "alloc.h"
#include "format.h"
#include "util.h"
#include "string.h"
#include "ops.h"
#include "env.h"
#include "runtime.h"
#include "dict.h"

#define stack_push(v, x) (v->stack[v->sp++] = x)
#define stack_pop(v) (v->stack[--v->sp])
#define stack_peek(v) (&v->stack[v->sp - 1])

vm_t *vm_create()
{
    vm_t *vm;

    vm = (vm_t *)rf_malloc(sizeof(struct vm_t));
    memset(vm, 0, sizeof(struct vm_t));

    vm->stack = (rf_object_t *)mmap(NULL, VM_STACK_SIZE,
                                    PROT_READ | PROT_WRITE,
                                    MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK | MAP_GROWSDOWN,
                                    -1, 0);

    return vm;
}

/*
 * Dispatch using computed goto technique
 */
rf_object_t vm_exec(vm_t *vm, str_t code)
{
    rf_object_t x, y, z, w, k, p, n, v, *addr;
    i32_t i;

    vm->ip = 0;
    vm->sp = 0;

    // The indices of labels in the dispatch_table are the relevant opcodes
    static null_t *dispatch_table[] = {
        &&op_halt, &&op_push, &&op_pop, &&op_addi, &&op_addf, &&op_subi, &&op_subf,
        &&op_muli, &&op_mulf, &&op_divi, &&op_divf, &&op_sumi, &&op_like, &&op_type,
        &&op_timer_set, &&op_timer_get, &&op_til, &&op_call1, &&op_call2, &&op_call3,
        &&op_call4, &&op_set, &&op_get};

#define dispatch() goto *dispatch_table[(i32_t)code[vm->ip]]

    dispatch();

op_halt:
    vm->halted = 1;
    if (vm->sp > 0)
        return stack_pop(vm);
    else
        return null();
op_push:
    vm->ip++;
    y = *(rf_object_t *)(code + vm->ip);
    stack_push(vm, y);
    vm->ip += sizeof(rf_object_t);
    dispatch();
op_pop:
    vm->ip++;
    y = stack_pop(vm);
    dispatch();
op_addi:
    vm->ip++;
    y = stack_pop(vm);
    x = stack_pop(vm);
    z = i64(ADDI64(x.i64, y.i64));
    stack_push(vm, z);
    dispatch();
op_addf:
    vm->ip++;
    y = stack_pop(vm);
    stack_peek(vm)->f64 += y.f64;
    dispatch();
op_subi:
    vm->ip++;
    y = stack_pop(vm);
    x = stack_pop(vm);
    z = i64(SUBI64(x.i64, y.i64));
    stack_push(vm, z);
    dispatch();
op_subf:
    vm->ip++;
    y = stack_pop(vm);
    stack_peek(vm)->f64 -= y.f64;
    dispatch();
op_muli:
    vm->ip++;
    y = stack_pop(vm);
    x = stack_pop(vm);
    z = i64(MULI64(x.i64, y.i64));
    stack_push(vm, z);
    dispatch();
op_mulf:
    vm->ip++;
    y = stack_pop(vm);
    stack_peek(vm)->f64 *= y.f64;
    dispatch();
op_divi:
    vm->ip++;
    y = stack_pop(vm);
    x = stack_pop(vm);
    z = f64(DIVI64(x.i64, y.i64));
    stack_push(vm, z);
    dispatch();
op_divf:
    vm->ip++;
    y = stack_pop(vm);
    x = stack_pop(vm);
    z = f64(DIVF64(x.f64, y.f64));
    stack_push(vm, z);
    dispatch();
op_sumi:
    vm->ip++;
    y = stack_pop(vm);
    x = stack_pop(vm);
    for (i = 0; i < x.adt->len; i++)
        as_vector_i64(&x)[i] = ADDI64(as_vector_i64(&x)[i], y.i64);
    stack_push(vm, x);
    dispatch();
op_like:
    vm->ip++;
    y = stack_pop(vm);
    x = stack_pop(vm);
    stack_push(vm, i64(string_match(as_string(&y), as_string(&x))));
    dispatch();
op_type:
    vm->ip++;
    y = stack_pop(vm);
    stack_push(vm, symbol(type_fmt(y.type)));
    dispatch();
op_timer_set:
    vm->ip++;
    vm->timer = clock();
    dispatch();
op_timer_get:
    vm->ip++;
    stack_push(vm, f64((((f64_t)(clock() - vm->timer)) / CLOCKS_PER_SEC) * 1000));
    dispatch();
op_til:
    vm->ip++;
    y = stack_pop(vm);
    x = vector_i64(y.i64);
    for (i = 0; i < y.i64; i++)
        as_vector_i64(&x)[i] = i;
    stack_push(vm, x);
    dispatch();
op_call1:
    vm->ip++;
    y = *(rf_object_t *)(code + vm->ip);
    vm->ip += sizeof(rf_object_t);
    x = stack_pop(vm);
    unary_t f = (unary_t)y.i64;
    z = f(&x);
    // TODO: unwind
    if (z.type == TYPE_ERROR)
    {
        z.id = y.id;
        return z;
    }
    stack_push(vm, z);
    dispatch();
op_call2:
    vm->ip++;
    y = *(rf_object_t *)(code + vm->ip);
    vm->ip += sizeof(rf_object_t);
    x = stack_pop(vm);
    z = stack_pop(vm);
    binary_t g = (binary_t)y.i64;
    w = g(&x, &z);
    // TODO: unwind
    if (w.type == TYPE_ERROR)
    {
        w.id = w.id;
        return w;
    }
    stack_push(vm, w);
    dispatch();
op_call3:
    vm->ip++;
    y = *(rf_object_t *)(code + vm->ip);
    vm->ip += sizeof(rf_object_t);
    x = stack_pop(vm);
    z = stack_pop(vm);
    w = stack_pop(vm);
    ternary_t h = (ternary_t)y.i64;
    v = h(&x, &z, &w);
    // TODO: unwind
    if (v.type == TYPE_ERROR)
    {
        v.id = y.id;
        return v;
    }
    stack_push(vm, v);
    dispatch();
op_call4:
    vm->ip++;
    y = *(rf_object_t *)(code + vm->ip);
    vm->ip += sizeof(rf_object_t);
    x = stack_pop(vm);
    z = stack_pop(vm);
    w = stack_pop(vm);
    v = stack_pop(vm);
    quaternary_t q = (quaternary_t)y.i64;
    p = q(&x, &z, &w, &v);
    // TODO: unwind
    if (p.type == TYPE_ERROR)
    {
        p.id = y.id;
        return p;
    }
    stack_push(vm, p);
    dispatch();
op_set:
    vm->ip++;
    x = *(rf_object_t *)(code + vm->ip);
    vm->ip += sizeof(rf_object_t);
    y = stack_pop(vm);
    env_set_variable(&runtime_get()->env, x, rf_object_clone(&y));
    stack_push(vm, y);
    dispatch();
op_get:
    vm->ip++;
    addr = (*(rf_object_t *)(code + vm->ip)).i64;
    vm->ip += sizeof(rf_object_t);
    stack_push(vm, rf_object_clone(addr));
    dispatch();
}

null_t vm_free(vm_t *vm)
{
    rf_free(vm);
}
