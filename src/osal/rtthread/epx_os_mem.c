/**
 * @file epx_os_mem.c
 * @brief RT-Thread heap: rt_malloc/rt_free with debug hooks.
 */

#include "osal/epx_os_mem.h"
#include "epx_config.h"

#include <rtthread.h>

void* epx_os_malloc(size_t size)
{
    void* ptr = rt_malloc(size);
    EPX_OS_MEM_DEBUG_HOOK_ALLOC(ptr, size);
    return ptr;
}

void epx_os_free(void* ptr)
{
    if (ptr != NULL) {
        EPX_OS_MEM_DEBUG_HOOK_FREE(ptr);
        rt_free(ptr);
    }
}
