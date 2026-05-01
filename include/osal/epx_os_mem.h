/**
 * @file epx_os_mem.h
 * @brief OSAL dynamic memory. Linux: malloc/free; RTOS: pvPortMalloc/vPortFree.
 *        Debug hooks reserved for leak tracking.
 */

#ifndef EPX_OS_MEM_H
#define EPX_OS_MEM_H

#include "epx_config.h"
#include "epx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(EPX_OSAL_ENABLE) && defined(EPX_OS_MEM_ENABLE)

/**
 * @brief Allocate memory. Use EPX_OS_MEM_DEBUG_HOOK_ALLOC in epx_config.h for tracking.
 * @param size  Size in bytes.
 * @return Pointer to memory, or NULL on failure.
 */
void* epx_os_malloc(size_t size);

/**
 * @brief Free memory. Use EPX_OS_MEM_DEBUG_HOOK_FREE in epx_config.h for tracking.
 * @param ptr  Pointer from epx_os_malloc (NULL is allowed, no-op).
 */
void epx_os_free(void* ptr);

#endif /* EPX_OSAL_ENABLE && EPX_OS_MEM_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_OS_MEM_H */
