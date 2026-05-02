/**
 * @file epx_os_mem.h
 * @brief OSAL 动态内存. Linux: malloc/free; RTOS: pvPortMalloc/vPortFree.
 *        预留调试钩子用于泄漏跟踪.
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
 * @brief 分配内存. 可在 epx_config.h 中用 EPX_OS_MEM_DEBUG_HOOK_ALLOC 跟踪.
 * @param size  字节数.
 * @return 内存指针, 失败返回 NULL.
 */
void* epx_os_malloc(size_t size);

/**
 * @brief 释放内存. 可在 epx_config.h 中用 EPX_OS_MEM_DEBUG_HOOK_FREE 跟踪.
 * @param ptr  epx_os_malloc 返回的指针 (允许 NULL, 空操作).
 */
void epx_os_free(void* ptr);

#endif /* EPX_OSAL_ENABLE && EPX_OS_MEM_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_OS_MEM_H */
