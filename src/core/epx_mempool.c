/**
 * @file epx_mempool.c
 * @brief Simple memory pool implementation using free list.
 */

#include "core/epx_mempool.h"
#include "osal/epx_os_mem.h"
#include "osal/epx_os_mutex.h"

#if defined(EPX_OSAL_ENABLE) && defined(EPX_OS_MEM_ENABLE)

#include <stdint.h>
#include <string.h>

/**
 * @brief Free list node embedded in each free block.
 */
typedef struct mempool_free_node {
    struct mempool_free_node* next;
} mempool_free_node_t;

/**
 * @brief Memory pool structure.
 */
struct epx_mempool {
    size_t block_size;
    uint32_t total_blocks;
    uint32_t free_count;
    mempool_free_node_t* free_list;
    void* memory;
    epx_os_mutex_t mutex;
};

epx_mempool_t epx_mempool_create(size_t block_size, uint32_t block_count)
{
    if (block_size < sizeof(mempool_free_node_t)) {
        block_size = sizeof(mempool_free_node_t);
    }
    /* Align block size to pointer alignment */
    size_t align = sizeof(void*);
    block_size = (block_size + align - 1) & ~(align - 1);

    size_t total_size = block_size * block_count;
    void* memory = epx_os_malloc(total_size);
    if (memory == NULL) {
        return NULL;
    }

    epx_mempool_t pool = (epx_mempool_t)epx_os_malloc(sizeof(struct epx_mempool));
    if (pool == NULL) {
        epx_os_free(memory);
        return NULL;
    }

    epx_os_mutex_t mutex = NULL;
    (void)epx_os_mutex_create(&mutex);

    /* Initialize free list */
    mempool_free_node_t* free_list = NULL;
    char* p = (char*)memory;
    for (uint32_t i = 0; i < block_count; i++) {
        mempool_free_node_t* node = (mempool_free_node_t*)p;
        node->next = free_list;
        free_list = node;
        p += block_size;
    }

    pool->block_size = block_size;
    pool->total_blocks = block_count;
    pool->free_count = block_count;
    pool->free_list = free_list;
    pool->memory = memory;
    pool->mutex = mutex;

    return pool;
}

void epx_mempool_destroy(epx_mempool_t pool)
{
    if (pool == NULL) {
        return;
    }
    if (pool->mutex != NULL) {
        epx_os_mutex_destroy(&pool->mutex);
    }
    if (pool->memory != NULL) {
        epx_os_free(pool->memory);
    }
    epx_os_free(pool);
}

void* epx_mempool_alloc(epx_mempool_t pool)
{
    if (pool == NULL || pool->free_list == NULL) {
        return NULL;
    }
    if (pool->mutex != NULL) {
        epx_os_mutex_lock(pool->mutex);
    }
    if (pool->free_list == NULL) {
        if (pool->mutex != NULL) {
            epx_os_mutex_unlock(pool->mutex);
        }
        return NULL;
    }
    mempool_free_node_t* node = pool->free_list;
    pool->free_list = node->next;
    pool->free_count--;
    if (pool->mutex != NULL) {
        epx_os_mutex_unlock(pool->mutex);
    }
    return (void*)node;
}

void epx_mempool_free(epx_mempool_t pool, void* ptr)
{
    if (pool == NULL || ptr == NULL) {
        return;
    }
    /* Optionally validate ptr is within pool bounds */
    char* mem = (char*)pool->memory;
    char* p = (char*)ptr;
    if (p < mem || p >= mem + pool->block_size * pool->total_blocks) {
        return;
    }
    if (pool->mutex != NULL) {
        epx_os_mutex_lock(pool->mutex);
    }
    mempool_free_node_t* node = (mempool_free_node_t*)ptr;
    node->next = pool->free_list;
    pool->free_list = node;
    pool->free_count++;
    if (pool->mutex != NULL) {
        epx_os_mutex_unlock(pool->mutex);
    }
}

uint32_t epx_mempool_free_count(epx_mempool_t pool)
{
    if (pool == NULL) {
        return 0;
    }
    return pool->free_count;
}

size_t epx_mempool_block_size(epx_mempool_t pool)
{
    if (pool == NULL) {
        return 0;
    }
    return pool->block_size;
}

#endif /* EPX_OSAL_ENABLE && EPX_OS_MEM_ENABLE */
