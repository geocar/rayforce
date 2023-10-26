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

#ifndef VM_H
#define VM_H

#include <setjmp.h>
#include <time.h>
#include "rayforce.h"
#include "lambda.h"
#include "mmap.h"
#include "nfo.h"

#define VM_STACK_SIZE PAGE_SIZE * 4

typedef struct vm_ctx_t
{
    i32_t bp;
    i32_t ip;
} vm_ctx_t;

typedef enum vm_opcode_t
{
    OP_RET = 0,    // Return
    OP_PUSH,       // Push an obj_t (from the lambda body) to the stack
    OP_PUSH_CONST, // Push an const obj_t (from the lambda constants) to the stack
    OP_POP,        // Pop an obj_t from the stack
    OP_SWAP,       // Swap two obj_ts on the stack
    OP_DUP,        // Duplicate an obj_t on the stack
    OP_JNE,        // Jump if not equal
    OP_JMP,        // Jump
    OP_CALL1,      // Call unary
    OP_CALL2,      // Call binary
    OP_CALLN,      // Call vary
    OP_CALLD,      // Dynamic call (call function from stack with n arguments)
    OP_TIMER_SET,  // Start timer
    OP_TIMER_GET,  // Get timer value
    OP_STORE,      // Store value somewhere in a stack pointed by argument
    OP_LOAD,       // Load value from somewhere in a stack pointed by argument
    OP_LSET,       // Set local variable
    OP_LGET,       // Get local variable
    OP_LPUSH,      // Attach dict frame to local variables
    OP_LPOP,       // Detach dict frame from local variables
    OP_TRY,        // Trap an expression to return here on error
    OP_CATCH,      // Catch an error from vm register and push it onto the stack
    OP_THROW,      // Throw an error
    OP_TRACE,      // Print stack trace (limit)

    OP_INVALID, // Invalid opcode
} vm_opcode_t;

typedef struct vm_t
{
    i32_t ip;     // Instruction pointer
    i32_t sp;     // Stack pointer
    i32_t bp;     // Base pointer (beginning on stack frame)
    i32_t timer;  // Timer for execution time
    jmp_buf jmp;  // Jump buffer for error handling.
    obj_t *stack; // Stack of arguments
} vm_t;

vm_t vm_new(obj_t *stack);
obj_t vm_exec(vm_t *vm, obj_t fun) __attribute__((__noinline__));
nil_t vm_free(vm_t *vm);
str_t vm_code_fmt(obj_t fun);
vm_ctx_t vm_save_ctx(vm_t *vm);
nil_t vm_restore_ctx(vm_t *vm, vm_ctx_t ctx);

#endif
