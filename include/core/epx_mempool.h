/**
 * @file epx_mempool.h
 * @brief Simple memory pool for fixed-size blocks to reduce malloc/free overhead.
 */

#ifndef EPX_MEMPOOL_H
#define EPX_MEMPOOL_H

#include "epx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque memory pool handle.
 */
typedef struct epx_mempool* epx_mempool_t;

#if defined(EPX_OSAL_ENABLE) && defined(EPX_OS_MEM_ENABLE)

/**
 * @brief Create a memory pool with specified block size and count.
 * @param block_size  Size of each block in bytes.
 * @param block_count Number of blocks in the pool.
 * @return Pool handle, or NULL on failure.
 */
epx_mempool_t epx_mempool_create(size_t block_size, uint32_t block_count);

/**
 * @brief Destroy a memory pool, free all blocks.
 * @param pool Pool handle (NULL is no-op).
 */
void epx_mempool_destroy(epx_mempool_t pool);

/**
 * @brief Allocate a block from pool. NULL if pool exhausted.
 * @param pool Pool handle.
 * @return Pointer to block, or NULL if no free blocks.
 */
void* epx_mempool_alloc(epx_mempool_t pool);

/**
 * @brief Return a block to pool.
 * @param pool Pool handle.
 * @param ptr  Block pointer from epx_mempool_alloc (NULL is no-op).
 */
void epx_mempool_free(epx_mempool_t pool, void* ptr);

/**
 * @brief Get number of free blocks in pool.
 * @param pool Pool handle.
 * @return Number of available blocks.
 */
uint32_t epx_mempool_free_count(epx_mempool_t pool);

#endif /* EPX_OSAL_ENABLE && EPX_OS_MEM_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_MEMPOOL_H */
