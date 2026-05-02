/**
 * @file epx_os_thread.c
 * @brief RT-Thread 线程: rt_thread_create, rt_thread_mdelay, 通过信号量 join.
 */

#include "osal/epx_os_thread.h"

#include <rtthread.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef EPX_RTTHREAD_STACK_DEFAULT
#define EPX_RTTHREAD_STACK_DEFAULT  (2048u)
#endif

struct epx_os_thread {
    rt_thread_t thread;
    rt_sem_t join_sem;
    void* exit_value;
    epx_os_thread_entry_t user_entry;
    void* user_arg;
    rt_bool_t joined;
};

static epx_time_t rtthread_ticks_to_ms(rt_tick_t ticks)
{
    rt_uint32_t tps = RT_TICK_PER_SECOND;
    if (tps == 0u) {
        return 0u;
    }
    return (epx_time_t)(((uint64_t)ticks * 1000ULL) / (uint64_t)tps);
}

static rt_uint8_t rtthread_map_prio(epx_os_prio_t prio)
{
    uint32_t p = (uint32_t)prio;
    if (p >= (uint32_t)EPX_OS_PRIO_COUNT) {
        p = (uint32_t)EPX_OS_PRIO_NORMAL;
    }
    if ((uint32_t)EPX_OS_PRIO_COUNT <= 1U) {
        return (rt_uint8_t)(RT_THREAD_PRIORITY_MAX / 2u);
    }
    uint32_t inv = (uint32_t)EPX_OS_PRIO_COUNT - 1U - p;
    rt_uint32_t max_rt = (rt_uint32_t)RT_THREAD_PRIORITY_MAX - 1U;
    return (rt_uint8_t)((inv * max_rt) / ((uint32_t)EPX_OS_PRIO_COUNT - 1U));
}

static void rtthread_entry(void* parameter)
{
    struct epx_os_thread* ctx = (struct epx_os_thread*)parameter;
    if (ctx != NULL) {
        ctx->exit_value = ctx->user_entry(ctx->user_arg);
        (void)rt_sem_release(ctx->join_sem);
    }
    (void)rt_thread_delete(rt_thread_self());
}

epx_err_t epx_os_init(void)
{
    return EPX_OK;
}

epx_time_t epx_os_get_time_ms(void)
{
    return rtthread_ticks_to_ms(rt_tick_get());
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
    struct epx_os_thread* ctx = (struct epx_os_thread*)rt_malloc(sizeof(struct epx_os_thread));
    if (ctx == NULL) {
        return EPX_ERR_NOMEM;
    }
    (void)memset(ctx, 0, sizeof(struct epx_os_thread));
    ctx->user_entry = entry;
    ctx->user_arg = arg;
    ctx->joined = RT_FALSE;
    ctx->join_sem = rt_sem_create("epx_j", 0, RT_IPC_FLAG_FIFO);
    if (ctx->join_sem == RT_NULL) {
        rt_free(ctx);
        return EPX_ERR_NOMEM;
    }

    rt_uint32_t stack = (stack_size == 0U) ? (rt_uint32_t)EPX_RTTHREAD_STACK_DEFAULT : (rt_uint32_t)stack_size;
    const char* thread_name = (name != NULL) ? name : "epx";
    ctx->thread = rt_thread_create(thread_name, rtthread_entry, ctx, stack, rtthread_map_prio(prio), 20);
    if (ctx->thread == RT_NULL) {
        (void)rt_sem_delete(ctx->join_sem);
        rt_free(ctx);
        return EPX_ERR_NOMEM;
    }
    (void)rt_thread_startup(ctx->thread);
    *handle = ctx;
    return EPX_OK;
}

epx_err_t epx_os_thread_join(epx_os_thread_handle_t handle, void** result)
{
    if (handle == NULL) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_thread* ctx = (struct epx_os_thread*)handle;
    if (ctx->joined == RT_TRUE) {
        if (result != NULL) {
            *result = ctx->exit_value;
        }
        return EPX_OK;
    }
    if (rt_sem_take(ctx->join_sem, RT_WAITING_FOREVER) != RT_EOK) {
        return EPX_ERR;
    }
    ctx->joined = RT_TRUE;
    if (result != NULL) {
        *result = ctx->exit_value;
    }
    return EPX_OK;
}

void epx_os_delay(uint32_t ms)
{
    (void)rt_thread_mdelay((rt_int32_t)ms);
}

void epx_os_thread_destroy(epx_os_thread_handle_t* handle)
{
    if (handle == NULL || *handle == NULL) {
        return;
    }
    struct epx_os_thread* ctx = (struct epx_os_thread*)*handle;
    if (ctx->join_sem != RT_NULL) {
        (void)rt_sem_delete(ctx->join_sem);
    }
    rt_free(ctx);
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
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    (void)vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1u] = '\0';
    rt_kprintf("[%lu.%03lu] [INFO] %s", sec, frac, buf);
}
