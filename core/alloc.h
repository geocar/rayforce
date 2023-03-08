#ifndef ALLOC_H_
#define ALLOC_H_

#include "storm.h"

#define PAGE_SIZE 4096
#define MIN_ORDER 2
#define MAX_ORDER 10
#define MIN_ALLOC ((i64_t)1 << MIN_ORDER)
#define MAX_ALLOC ((i64_t)1 << MAX_ORDER)
#define POOL_SIZE (MAX_ALLOC * 2)
#define cell_COUNT (MAX_ORDER - MIN_ORDER + 1)

typedef struct alloc_t
{
    null_t *freelist[cell_COUNT];
    i8_t pool[POOL_SIZE];
} __attribute__((aligned(PAGE_SIZE))) * alloc_t;

CASSERT(sizeof(struct alloc_t) % PAGE_SIZE == 0, alloc_h)

extern null_t *storm_malloc(i32_t size);
extern null_t *storm_realloc(null_t *ptr, i32_t size);
extern null_t storm_free(null_t *block);

extern null_t storm_alloc_init();
extern null_t storm_alloc_deinit();

#endif
