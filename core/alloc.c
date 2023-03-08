#include <stdio.h>
#include <string.h>
#include "alloc.h"
#include "storm.h"
#include <sys/mman.h>
#include <assert.h>

#include <stdlib.h>

#define MAP_ANONYMOUS 0x20

// Global allocator reference
alloc_t GLOBAL_A0 = NULL;

extern null_t *storm_malloc(int size)
{
    return malloc(size);
}

extern null_t storm_free(void *block)
{
    free(block);
}

extern null_t *storm_realloc(null_t *ptr, i32_t size)
{
    return realloc(ptr, size);
}

extern null_t storm_alloc_init()
{
}

extern null_t storm_alloc_deinit()
{
    // munmap(GLOBAL_A0, sizeof(struct alloc_t));
}
