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
#include <assert.h>
#include "alloc.h"
#include "rayforce.h"
#include "ops.h"
#include "mmap.h"
#include "util.h"

#include <stdlib.h>
#include <stdint.h>
#include <math.h>

static alloc_t _ALLOC = NULL;

#define MEMORY_SIZE (1024 * 1024 * 1024) // 1MB of memory
#define MIN_BLOCK_SIZE 64

typedef struct BuddyNode
{
    i32_t size;
    bool_t is_free;
    struct BuddyNode *next;
} BuddyNode;

static BuddyNode *head = NULL;

alloc_t rf_alloc_init()
{
    _ALLOC = (alloc_t)mmap(NULL, sizeof(struct alloc_t),
                           PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    head = (BuddyNode *)mmap(NULL, MEMORY_SIZE,
                             PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ;
    head->size = MEMORY_SIZE;
    head->is_free = true;
    head->next = NULL;

    return _ALLOC;
}

alloc_t rf_alloc_get()
{
    return _ALLOC;
}

null_t rf_alloc_cleanup()
{
    munmap(_ALLOC, sizeof(struct alloc_t));
    munmap(head, MEMORY_SIZE);
}

null_t split_block(BuddyNode *block)
{
    i32_t half_size = block->size / 2;

    BuddyNode *new_block = (BuddyNode *)((uint8_t *)block + half_size);
    new_block->size = half_size;
    new_block->is_free = true;
    new_block->next = block->next;

    block->size = half_size;
    block->next = new_block;
}

BuddyNode *find_best_fit(i32_t size)
{
    BuddyNode *prev = NULL;
    BuddyNode *best_fit = NULL;
    BuddyNode *best_fit_prev = NULL;
    BuddyNode *current = head;
    while (current != NULL)
    {
        if (current->is_free && current->size >= size)
        {
            if (best_fit == NULL || current->size < best_fit->size)
            {
                best_fit = current;
                best_fit_prev = prev;
            }
        }

        prev = current;
        current = current->next;
    }

    debug("RET BEST FIT: %p", best_fit_prev);

    return best_fit_prev;
}

null_t *rf_malloc(i32_t size)
{
    if (size == 0 || size > MEMORY_SIZE)
        return NULL;

    i32_t requested_size = sizeof(BuddyNode) + size;
    i32_t block_size = MIN_BLOCK_SIZE;

    while (block_size < requested_size)
        block_size *= 2;

    BuddyNode *prev = find_best_fit(block_size);
    BuddyNode *current = prev ? prev->next : head;

    if (current == NULL)
        return NULL;

    while (current->size > block_size)
        split_block(current);

    current->is_free = false;
    return current + sizeof(BuddyNode);
}

null_t rf_free(void *ptr)
{
    if (ptr == NULL)
        return;

    BuddyNode *block = (BuddyNode *)((uint8_t *)ptr - sizeof(BuddyNode));
    block->is_free = true;
}

null_t *rf_realloc(null_t *ptr, i32_t new_size)
{
    if (ptr == NULL)
        return rf_malloc(new_size);

    if (new_size == 0)
    {
        rf_free(ptr);
        return NULL;
    }

    BuddyNode *block = (BuddyNode *)((uint8_t *)ptr - sizeof(BuddyNode));
    i32_t current_size = block->size - sizeof(BuddyNode);

    if (new_size <= current_size)
        return ptr;

    void *new_ptr = rf_malloc(new_size);
    if (new_ptr != NULL)
    {
        memcpy(new_ptr, ptr, current_size);
        rf_free(ptr);
    }

    return new_ptr;
}
