/**
 * @file epx_mempool.h
 * @brief 定长块内存池, 降低 malloc/free 开销.
 */

#ifndef EPX_MEMPOOL_H
#define EPX_MEMPOOL_H

#include "epx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 不透明内存池句柄.
 */
typedef struct epx_mempool* epx_mempool_t;

#if defined(EPX_OSAL_ENABLE) && defined(EPX_OS_MEM_ENABLE)

/**
 * @brief 创建指定块大小与数量的内存池.
 * @param block_size  每块字节数.
 * @param block_count 池中块数量.
 * @return 池句柄, 失败返回 NULL.
 */
epx_mempool_t epx_mempool_create(size_t block_size, uint32_t block_count);

/**
 * @brief 销毁内存池并释放所有块.
 * @param pool 池句柄 (NULL 时不做任何操作).
 */
void epx_mempool_destroy(epx_mempool_t pool);

/**
 * @brief 从池中分配一块. 池耗尽时返回 NULL.
 * @param pool 池句柄.
 * @return 块指针, 无空闲块时返回 NULL.
 */
void* epx_mempool_alloc(epx_mempool_t pool);

/**
 * @brief 将块归还池中.
 * @param pool 池句柄.
 * @param ptr  由 epx_mempool_alloc 返回的块指针 (NULL 时不做任何操作).
 */
void epx_mempool_free(epx_mempool_t pool, void* ptr);

/**
 * @brief 获取池中空闲块数量.
 * @param pool 池句柄.
 * @return 可用块数量.
 */
uint32_t epx_mempool_free_count(epx_mempool_t pool);

/**
 * @brief 池中单个槽位的有效块大小 (对齐后), 单位字节.
 * @param pool 池句柄 (NULL 时返回 0).
 * @return 块大小, 单位字节.
 */
size_t epx_mempool_block_size(epx_mempool_t pool);

#endif /* EPX_OSAL_ENABLE && EPX_OS_MEM_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_MEMPOOL_H */
