/**
 * @file epx_os_thread.c
 * @brief POSIX implementation: pthread, nanosleep.
 */

#include "osal/epx_os_thread.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

struct epx_os_thread {
    pthread_t thread;
    epx_bool_t joined;
};

epx_err_t epx_os_init(void)
{
    return EPX_OK;
}

epx_time_t epx_os_get_time_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0u;
    }
    return (epx_time_t)((uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000));
}

epx_err_t epx_os_thread_create(epx_os_thread_handle_t* handle,
                               const char* name,
                               uint32_t stack_size,
                               epx_os_prio_t prio,
                               epx_os_thread_entry_t entry,
                               void* arg)
{
    if (handle == NULL || entry == NULL) {
        return EPX_ERR_PARAM;
    }
    (void)name;
    (void)stack_size;
    (void)prio;
    struct epx_os_thread* ctx = (struct epx_os_thread*)malloc(sizeof(struct epx_os_thread));
    if (ctx == NULL) {
        return EPX_ERR_NOMEM;
    }
    memset(ctx, 0, sizeof(struct epx_os_thread));
    ctx->joined = EPX_FALSE;

    int ret = pthread_create(&ctx->thread, NULL, entry, arg);
    if (ret != 0) {
        free(ctx);
        return EPX_ERR_NOMEM;
    }
    *handle = (epx_os_thread_handle_t)ctx;
    return EPX_OK;
}

epx_err_t epx_os_thread_join(epx_os_thread_handle_t handle, void** result)
{
    if (handle == NULL) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_thread* ctx = (struct epx_os_thread*)handle;
    if (ctx->joined) {
        return EPX_OK;
    }
    void* join_ret = NULL;
    int ret = pthread_join(ctx->thread, &join_ret);
    if (ret != 0) {
        return EPX_ERR;
    }
    ctx->joined = EPX_TRUE;
    if (result != NULL) {
        *result = join_ret;
    }
    return EPX_OK;
}

void epx_os_delay(uint32_t ms)
{
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000u);
    ts.tv_nsec = (long)((ms % 1000u) * 1000000u);
    nanosleep(&ts, NULL);
}

void epx_os_thread_destroy(epx_os_thread_handle_t* handle)
{
    if (handle == NULL || *handle == NULL) {
        return;
    }
    free(*handle);
    *handle = NULL;
}

void epx_os_printf(const char* fmt, ...)
{
    if (fmt == NULL) {
        return;
    }
    epx_time_t ms = epx_os_get_time_ms();
    unsigned long sec = (unsigned long)(ms / 1000u);
    unsigned long frac = (unsigned long)(ms % 1000u);
    (void)fprintf(stdout, "[%lu.%03lu] [INFO] ", sec, frac);
    va_list ap;
    va_start(ap, fmt);
    (void)vfprintf(stdout, fmt, ap);
    va_end(ap);
}
