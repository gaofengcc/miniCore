/**
 * @file epx_os_mutex.c
 * @brief FreeRTOS recursive mutex (xSemaphoreCreateRecursiveMutex).
 */

#include "osal/epx_os_mutex.h"

#include "FreeRTOS.h"
#include "semphr.h"

struct epx_os_mutex {
    SemaphoreHandle_t mtx;
};

epx_err_t epx_os_mutex_create(epx_os_mutex_t* mutex)
{
    if (mutex == NULL) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_mutex* ctx = (struct epx_os_mutex*)pvPortMalloc(sizeof(struct epx_os_mutex));
    if (ctx == NULL) {
        return EPX_ERR_NOMEM;
    }
    ctx->mtx = xSemaphoreCreateRecursiveMutex();
    if (ctx->mtx == NULL) {
        vPortFree(ctx);
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
    if (xSemaphoreTakeRecursive(ctx->mtx, portMAX_DELAY) == pdTRUE) {
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
    if (xSemaphoreGiveRecursive(ctx->mtx) == pdTRUE) {
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
    vSemaphoreDelete(ctx->mtx);
    vPortFree(ctx);
    *mutex = NULL;
}
