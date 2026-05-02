/**
 * @file epx_os_queue.c
 * @brief POSIX 实现: 环形缓冲 + 互斥 + 条件变量, 支持超时.
 */

#include "osal/epx_os_queue.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

struct epx_os_queue {
    size_t msg_len;
    uint32_t depth;
    uint8_t* ring;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    pthread_mutex_t mutex;
    pthread_cond_t cond_not_empty;
    pthread_cond_t cond_not_full;
};

epx_err_t epx_os_queue_create(epx_os_queue_t* queue, size_t msg_len, uint32_t depth)
{
    if (queue == NULL || msg_len == 0 || depth == 0) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_queue* ctx = (struct epx_os_queue*)malloc(sizeof(struct epx_os_queue));
    if (ctx == NULL) {
        return EPX_ERR_NOMEM;
    }
    ctx->msg_len = msg_len;
    ctx->depth = depth;
    ctx->ring = (uint8_t*)malloc(msg_len * depth);
    if (ctx->ring == NULL) {
        free(ctx);
        return EPX_ERR_NOMEM;
    }
    ctx->head = 0;
    ctx->tail = 0;
    ctx->count = 0;
    pthread_mutex_init(&ctx->mutex, NULL);
    pthread_cond_init(&ctx->cond_not_empty, NULL);
    pthread_cond_init(&ctx->cond_not_full, NULL);
    *queue = (epx_os_queue_t)ctx;
    return EPX_OK;
}

epx_err_t epx_os_queue_send(epx_os_queue_t queue, const void* msg, uint32_t timeout_ms)
{
    if (queue == NULL || msg == NULL) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_queue* ctx = (struct epx_os_queue*)queue;
    struct timespec abstime;
    int wait_forever = (timeout_ms == EPX_OS_QUEUE_WAIT_FOREVER);

    pthread_mutex_lock(&ctx->mutex);
    while (ctx->count >= ctx->depth) {
        if (!wait_forever) {
            clock_gettime(CLOCK_REALTIME, &abstime);
            abstime.tv_sec += (time_t)(timeout_ms / 1000u);
            abstime.tv_nsec += (long)((timeout_ms % 1000u) * 1000000u);
            if (abstime.tv_nsec >= 1000000000L) {
                abstime.tv_sec++;
                abstime.tv_nsec -= 1000000000L;
            }
            int ret = pthread_cond_timedwait(&ctx->cond_not_full, &ctx->mutex, &abstime);
            if (ret == ETIMEDOUT) {
                pthread_mutex_unlock(&ctx->mutex);
                return EPX_ERR_TIMEOUT;
            }
        } else {
            pthread_cond_wait(&ctx->cond_not_full, &ctx->mutex);
        }
    }
    memcpy(ctx->ring + ctx->tail * ctx->msg_len, msg, ctx->msg_len);
    ctx->tail = (ctx->tail + 1) % ctx->depth;
    ctx->count++;
    pthread_cond_signal(&ctx->cond_not_empty);
    pthread_mutex_unlock(&ctx->mutex);
    return EPX_OK;
}

epx_err_t epx_os_queue_recv(epx_os_queue_t queue, void* msg, uint32_t timeout_ms)
{
    if (queue == NULL || msg == NULL) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_queue* ctx = (struct epx_os_queue*)queue;
    struct timespec abstime;
    int wait_forever = (timeout_ms == EPX_OS_QUEUE_WAIT_FOREVER);

    pthread_mutex_lock(&ctx->mutex);
    while (ctx->count == 0) {
        if (!wait_forever) {
            clock_gettime(CLOCK_REALTIME, &abstime);
            abstime.tv_sec += (time_t)(timeout_ms / 1000u);
            abstime.tv_nsec += (long)((timeout_ms % 1000u) * 1000000u);
            if (abstime.tv_nsec >= 1000000000L) {
                abstime.tv_sec++;
                abstime.tv_nsec -= 1000000000L;
            }
            int ret = pthread_cond_timedwait(&ctx->cond_not_empty, &ctx->mutex, &abstime);
            if (ret == ETIMEDOUT) {
                pthread_mutex_unlock(&ctx->mutex);
                return EPX_ERR_TIMEOUT;
            }
        } else {
            pthread_cond_wait(&ctx->cond_not_empty, &ctx->mutex);
        }
    }
    memcpy(msg, ctx->ring + ctx->head * ctx->msg_len, ctx->msg_len);
    ctx->head = (ctx->head + 1) % ctx->depth;
    ctx->count--;
    pthread_cond_signal(&ctx->cond_not_full);
    pthread_mutex_unlock(&ctx->mutex);
    return EPX_OK;
}

void epx_os_queue_destroy(epx_os_queue_t* queue)
{
    if (queue == NULL || *queue == NULL) {
        return;
    }
    struct epx_os_queue* ctx = (struct epx_os_queue*)*queue;
    pthread_mutex_destroy(&ctx->mutex);
    pthread_cond_destroy(&ctx->cond_not_empty);
    pthread_cond_destroy(&ctx->cond_not_full);
    free(ctx->ring);
    free(ctx);
    *queue = NULL;
}

