/**
 * @file epx_msg.c
 * @brief 引用计数消息块. 通过 epx_os_mutex 保证线程安全.
 *        支持小消息内存池与零拷贝 topic 存储.
 */

#include "core/epx_msg.h"
#include "core/epx_mempool.h"
#include "epx_config.h"
#include "osal/epx_os_mem.h"
#include "osal/epx_os_mutex.h"

#if defined(EPX_CORE_ENABLE)

#include <stdlib.h>
#include <string.h>

/* 小消息默认池参数 */
#define EPX_MSG_POOL_DEFAULT_BLOCK_SIZE  256
#define EPX_MSG_POOL_DEFAULT_BLOCK_COUNT  64

struct epx_msg_block {
    volatile int ref_count;
    size_t size;
    char* topic;           /* 零拷贝: 直接存指针, 不拷贝 */
    uint8_t topic_copy : 1; /* 1 = topic 已拷贝, 0 = 零拷贝指针 */
    uint8_t from_pool : 1; /* 1 = 自内存池分配 */
    void (*destructor)(void*);
    uint8_t data[];
};

static epx_os_mutex_t g_msg_mutex = NULL;
static epx_mempool_t g_msg_pool = NULL;
/** 内存池有效块大小 (对齐后), 无池时为 0. */
static size_t g_msg_pool_block_bytes = 0;

/** 头与负载合计分配字节数 (柔性数组成员布局). */
static size_t msg_bytes_for_payload(size_t payload_len)
{
    return sizeof(struct epx_msg_block) + payload_len;
}

epx_err_t epx_msg_init(void)
{
    if (g_msg_mutex != NULL) {
        return EPX_OK;
    }
    epx_err_t ret = epx_os_mutex_create(&g_msg_mutex);
    if (ret != EPX_OK) {
        return ret;
    }
    /* 为小消息创建默认内存池 */
    g_msg_pool = epx_mempool_create(EPX_MSG_POOL_DEFAULT_BLOCK_SIZE, EPX_MSG_POOL_DEFAULT_BLOCK_COUNT);
    if (g_msg_pool != NULL) {
        g_msg_pool_block_bytes = epx_mempool_block_size(g_msg_pool);
    } else {
        g_msg_pool_block_bytes = 0;
    }
    return EPX_OK;
}

epx_err_t epx_msg_init_with_pool(size_t pool_block_size, uint32_t pool_block_count)
{
    if (g_msg_mutex != NULL) {
        return EPX_OK;
    }
    epx_err_t ret = epx_os_mutex_create(&g_msg_mutex);
    if (ret != EPX_OK) {
        return ret;
    }
    if (pool_block_size > 0 && pool_block_count > 0) {
        g_msg_pool = epx_mempool_create(pool_block_size, pool_block_count);
        if (g_msg_pool != NULL) {
            g_msg_pool_block_bytes = epx_mempool_block_size(g_msg_pool);
        } else {
            g_msg_pool_block_bytes = 0;
        }
    }
    return EPX_OK;
}

epx_msg_t epx_msg_alloc(size_t size)
{
    if (g_msg_mutex == NULL) {
        (void)epx_msg_init();
    }
    if (g_msg_mutex == NULL) {
        return NULL;
    }
    struct epx_msg_block* block = NULL;
    /* 单块可容纳时优先从内存池分配 */
    if (g_msg_pool != NULL && g_msg_pool_block_bytes > 0 &&
        msg_bytes_for_payload(size) <= g_msg_pool_block_bytes) {
        block = (struct epx_msg_block*)epx_mempool_alloc(g_msg_pool);
        if (block != NULL) {
            block->from_pool = 1;
        }
    }
    if (block == NULL) {
        block = (struct epx_msg_block*)epx_os_malloc(sizeof(struct epx_msg_block) + size);
        if (block == NULL) {
            return NULL;
        }
        block->from_pool = 0;
    }
    block->ref_count = 1;
    block->size = size;
    block->topic = NULL;
    block->topic_copy = 0;
    block->destructor = NULL;
    return (epx_msg_t)block;
}

epx_msg_t epx_msg_new(const char* topic, size_t payload_len)
{
    if (topic == NULL || topic[0] == '\0') {
        return NULL;
    }
    if (g_msg_mutex == NULL) {
        (void)epx_msg_init();
    }
    if (g_msg_mutex == NULL) {
        return NULL;
    }
    size_t topic_len = strlen(topic);
    if (topic_len >= (size_t)EPX_TOPIC_STR_MAX_LEN) {
        return NULL;
    }
    struct epx_msg_block* block = NULL;
    if (g_msg_pool != NULL && g_msg_pool_block_bytes > 0 &&
        msg_bytes_for_payload(payload_len) <= g_msg_pool_block_bytes) {
        block = (struct epx_msg_block*)epx_mempool_alloc(g_msg_pool);
        if (block != NULL) {
            block->from_pool = 1;
        }
    }
    if (block == NULL) {
        block = (struct epx_msg_block*)epx_os_malloc(sizeof(struct epx_msg_block) + payload_len);
        if (block == NULL) {
            return NULL;
        }
        block->from_pool = 0;
    }
    /* 零拷贝: 直接保存 topic 指针, 不拷贝 */
    block->topic = (char*)topic;
    block->topic_copy = 0;
    block->ref_count = 1;
    block->size = payload_len;
    block->destructor = NULL;
    return (epx_msg_t)block;
}

epx_msg_t epx_msg_new_copy(const char* topic, size_t payload_len)
{
    if (topic == NULL || topic[0] == '\0') {
        return NULL;
    }
    if (g_msg_mutex == NULL) {
        (void)epx_msg_init();
    }
    if (g_msg_mutex == NULL) {
        return NULL;
    }
    size_t topic_len = strlen(topic);
    if (topic_len >= (size_t)EPX_TOPIC_STR_MAX_LEN) {
        return NULL;
    }
    struct epx_msg_block* block = NULL;
    if (g_msg_pool != NULL && g_msg_pool_block_bytes > 0 &&
        msg_bytes_for_payload(payload_len) <= g_msg_pool_block_bytes) {
        block = (struct epx_msg_block*)epx_mempool_alloc(g_msg_pool);
        if (block != NULL) {
            block->from_pool = 1;
        }
    }
    if (block == NULL) {
        block = (struct epx_msg_block*)epx_os_malloc(sizeof(struct epx_msg_block) + payload_len);
        if (block == NULL) {
            return NULL;
        }
        block->from_pool = 0;
    }
    /* 兼容: 拷贝 topic 字符串 */
    block->topic = (char*)epx_os_malloc(topic_len + 1);
    if (block->topic == NULL) {
        if (block->from_pool) {
            epx_mempool_free(g_msg_pool, block);
        } else {
            epx_os_free(block);
        }
        return NULL;
    }
    memcpy(block->topic, topic, topic_len + 1);
    block->topic_copy = 1;
    block->ref_count = 1;
    block->size = payload_len;
    block->destructor = NULL;
    return (epx_msg_t)block;
}

void epx_msg_retain(epx_msg_t msg)
{
    if (msg == NULL) {
        return;
    }
    struct epx_msg_block* block = (struct epx_msg_block*)msg;
    epx_os_mutex_lock(g_msg_mutex);
    block->ref_count++;
    epx_os_mutex_unlock(g_msg_mutex);
}

void epx_msg_release(epx_msg_t msg)
{
    if (msg == NULL) {
        return;
    }
    struct epx_msg_block* block = (struct epx_msg_block*)msg;
    epx_os_mutex_lock(g_msg_mutex);
    block->ref_count--;
    int ref = block->ref_count;
    epx_os_mutex_unlock(g_msg_mutex);
    if (ref == 0) {
        if (block->destructor != NULL) {
            block->destructor(block);
        }
        /* 仅在已拷贝 topic 时释放 (非零拷贝) */
        if (block->topic != NULL && block->topic_copy) {
            epx_os_free(block->topic);
        }
        /* 归还池或释放到堆 */
        if (block->from_pool && g_msg_pool != NULL) {
            epx_mempool_free(g_msg_pool, block);
        } else {
            epx_os_free(block);
        }
    }
}

void epx_msg_set_destructor(epx_msg_t msg, void (*destructor)(void*))
{
    if (msg == NULL) {
        return;
    }
    ((struct epx_msg_block*)msg)->destructor = destructor;
}

void* epx_msg_payload(epx_msg_t msg)
{
    if (msg == NULL) {
        return NULL;
    }
    return ((struct epx_msg_block*)msg)->data;
}

size_t epx_msg_size(epx_msg_t msg)
{
    if (msg == NULL) {
        return 0;
    }
    return ((struct epx_msg_block*)msg)->size;
}

const char* epx_msg_topic(epx_msg_t msg)
{
    if (msg == NULL) {
        return NULL;
    }
    return (const char*)((struct epx_msg_block*)msg)->topic;
}

#endif /* EPX_CORE_ENABLE */
