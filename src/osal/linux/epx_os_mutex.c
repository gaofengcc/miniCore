/**
 * @file epx_os_mutex.c
 * @brief POSIX implementation: recursive mutex via pthread_mutexattr_settype(RECURSIVE).
 */

#include "osal/epx_os_mutex.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct epx_os_mutex {
    pthread_mutex_t mutex;
};

epx_err_t epx_os_mutex_create(epx_os_mutex_t* mutex)
{
    if (mutex == NULL) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_mutex* ctx = (struct epx_os_mutex*)malloc(sizeof(struct epx_os_mutex));
    if (ctx == NULL) {
        return EPX_ERR_NOMEM;
    }
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    int ret = pthread_mutex_init(&ctx->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    if (ret != 0) {
        free(ctx);
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
    int ret = pthread_mutex_lock(&ctx->mutex);
    return (ret == 0) ? EPX_OK : EPX_ERR;
}

epx_err_t epx_os_mutex_unlock(epx_os_mutex_t mutex)
{
    if (mutex == NULL) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_mutex* ctx = (struct epx_os_mutex*)mutex;
    int ret = pthread_mutex_unlock(&ctx->mutex);
    return (ret == 0) ? EPX_OK : EPX_ERR;
}

void epx_os_mutex_destroy(epx_os_mutex_t* mutex)
{
    if (mutex == NULL || *mutex == NULL) {
        return;
    }
    struct epx_os_mutex* ctx = (struct epx_os_mutex*)*mutex;
    pthread_mutex_destroy(&ctx->mutex);
    free(ctx);
    *mutex = NULL;
}
