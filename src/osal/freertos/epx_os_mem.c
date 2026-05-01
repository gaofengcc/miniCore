/**
 * @file epx_os_mem.c
 * @brief FreeRTOS heap: pvPortMalloc/vPortFree with debug hooks.
 */

#include "osal/epx_os_mem.h"
#include "epx_config.h"

#include "FreeRTOS.h"
#include "task.h"

void* epx_os_malloc(size_t size)
{
    void* ptr = pvPortMalloc(size);
    EPX_OS_MEM_DEBUG_HOOK_ALLOC(ptr, size);
    return ptr;
}

void epx_os_free(void* ptr)
{
    if (ptr != NULL) {
        EPX_OS_MEM_DEBUG_HOOK_FREE(ptr);
        vPortFree(ptr);
    }
}
