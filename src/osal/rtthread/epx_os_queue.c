/**
 * @file epx_os_queue.c
 * @brief RT-Thread message queue (rt_mq_*).
 */

#include "osal/epx_os_queue.h"

#include <rtthread.h>

#include <string.h>

struct epx_os_queue {
    rt_mq_t mq;
    rt_uint32_t msg_len;
};

epx_err_t epx_os_queue_create(epx_os_queue_t* queue, size_t msg_len, uint32_t depth)
{
    if (queue == NULL || msg_len == 0 || depth == 0) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_queue* ctx = (struct epx_os_queue*)rt_malloc(sizeof(struct epx_os_queue));
    if (ctx == NULL) {
        return EPX_ERR_NOMEM;
    }
    (void)memset(ctx, 0, sizeof(struct epx_os_queue));
    ctx->msg_len = (rt_uint32_t)msg_len;
    ctx->mq = rt_mq_create("epx_q", (rt_uint32_t)msg_len, depth, RT_IPC_FLAG_FIFO);
    if (ctx->mq == RT_NULL) {
        rt_free(ctx);
        return EPX_ERR_NOMEM;
    }
    *queue = (epx_os_queue_t)ctx;
    return EPX_OK;
}

epx_err_t epx_os_queue_send(epx_os_queue_t queue, const void* msg, uint32_t timeout_ms)
{
    if (queue == NULL || msg == NULL) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_queue* ctx = (struct epx_os_queue*)queue;
    rt_int32_t ticks;
    if (timeout_ms == EPX_OS_QUEUE_WAIT_FOREVER) {
        ticks = RT_WAITING_FOREVER;
    } else {
        ticks = (rt_int32_t)rt_tick_from_millisecond(timeout_ms);
    }
    if (rt_mq_send_wait(ctx->mq, (void*)msg, ctx->msg_len, ticks) == RT_EOK) {
        return EPX_OK;
    }
    return EPX_ERR_TIMEOUT;
}

epx_err_t epx_os_queue_recv(epx_os_queue_t queue, void* msg, uint32_t timeout_ms)
{
    if (queue == NULL || msg == NULL) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_queue* ctx = (struct epx_os_queue*)queue;
    rt_int32_t ticks;
    if (timeout_ms == EPX_OS_QUEUE_WAIT_FOREVER) {
        ticks = RT_WAITING_FOREVER;
    } else {
        ticks = (rt_int32_t)rt_tick_from_millisecond(timeout_ms);
    }
    rt_ssize_t n = rt_mq_recv(ctx->mq, msg, ctx->msg_len, ticks);
    if (n >= 0) {
        return EPX_OK;
    }
    return EPX_ERR_TIMEOUT;
}

void epx_os_queue_destroy(epx_os_queue_t* queue)
{
    if (queue == NULL || *queue == NULL) {
        return;
    }
    struct epx_os_queue* ctx = (struct epx_os_queue*)*queue;
    (void)rt_mq_delete(ctx->mq);
    rt_free(ctx);
    *queue = NULL;
}
