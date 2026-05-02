/**
 * @file epx_os_mem.c
 * @brief POSIX 实现: malloc/free, 带调试钩子.
 */

#include "osal/epx_os_mem.h"
#include "epx_config.h"
#include <stdlib.h>

void* epx_os_malloc(size_t size)
{
    void* ptr = malloc(size);
    EPX_OS_MEM_DEBUG_HOOK_ALLOC(ptr, size);
    return ptr;
}

void epx_os_free(void* ptr)
{
    if (ptr != NULL) {
        EPX_OS_MEM_DEBUG_HOOK_FREE(ptr);
        free(ptr);
    }
}

