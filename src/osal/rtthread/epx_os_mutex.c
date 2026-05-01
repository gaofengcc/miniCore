/**
 * @file epx_os_mutex.c
 * @brief RT-Thread mutex (recursive take/release).
 */

#include "osal/epx_os_mutex.h"

#include <rtthread.h>

#include <stdlib.h>
#include <string.h>

struct epx_os_mutex {
    rt_mutex_t mtx;
};

epx_err_t epx_os_mutex_create(epx_os_mutex_t* mutex)
{
    if (mutex == NULL) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_mutex* ctx = (struct epx_os_mutex*)rt_malloc(sizeof(struct epx_os_mutex));
    if (ctx == NULL) {
        return EPX_ERR_NOMEM;
    }
    (void)memset(ctx, 0, sizeof(struct epx_os_mutex));
    ctx->mtx = rt_mutex_create("epx_mtx", RT_IPC_FLAG_PRIO);
    if (ctx->mtx == RT_NULL) {
        rt_free(ctx);
        return EPX_ERR_NOMEM;
    }
    *mutex = (epx_os_mutex_t)ctx;
    return EPX_OK;
}

epx_err_t epx_os_mutex_lock(epx_os_mutex_t mutex)
{
    if (mutex == NULL) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_mutex* ctx = (struct epx_os_mutex*)mutex;
    if (rt_mutex_take(ctx->mtx, RT_WAITING_FOREVER) == RT_EOK) {
        return EPX_OK;
    }
    return EPX_ERR;
}

epx_err_t epx_os_mutex_unlock(epx_os_mutex_t mutex)
{
    if (mutex == NULL) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_mutex* ctx = (struct epx_os_mutex*)mutex;
    if (rt_mutex_release(ctx->mtx) == RT_EOK) {
        return EPX_OK;
    }
    return EPX_ERR;
}

void epx_os_mutex_destroy(epx_os_mutex_t* mutex)
{
    if (mutex == NULL || *mutex == NULL) {
        return;
    }
    struct epx_os_mutex* ctx = (struct epx_os_mutex*)*mutex;
    (void)rt_mutex_delete(ctx->mtx);
    rt_free(ctx);
    *mutex = NULL;
}
