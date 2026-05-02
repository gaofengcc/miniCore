/**
 * @file epx_os_thread.c
 * @brief FreeRTOS 任务: xTaskCreate, vTaskDelay, 通过二值信号量 join.
 */

#include "osal/epx_os_thread.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef EPX_FREERTOS_STACK_WORDS_MIN
#define EPX_FREERTOS_STACK_WORDS_MIN  ((uint32_t)128U)
#endif

struct epx_os_thread {
    TaskHandle_t task;
    SemaphoreHandle_t join_sem;
    void* exit_value;
    epx_os_thread_entry_t user_entry;
    void* user_arg;
    epx_bool_t joined;
};

static epx_time_t freertos_ticks_to_ms(TickType_t ticks)
{
    uint32_t hz = (uint32_t)configTICK_RATE_HZ;
    if (hz == 0U) {
        return 0u;
    }
    return (epx_time_t)(((uint64_t)ticks * 1000ULL) / (uint64_t)hz);
}

static UBaseType_t freertos_map_prio(epx_os_prio_t prio)
{
    uint32_t p = (uint32_t)prio;
    if (p >= (uint32_t)EPX_OS_PRIO_COUNT) {
        p = (uint32_t)EPX_OS_PRIO_NORMAL;
    }
    if ((uint32_t)EPX_OS_PRIO_COUNT <= 1U) {
        return (UBaseType_t)1U;
    }
    UBaseType_t span = (UBaseType_t)((uint32_t)configMAX_PRIORITIES > 0U ? (uint32_t)configMAX_PRIORITIES - 1U : 1U);
    uint32_t num = p * (uint32_t)span;
    uint32_t den = (uint32_t)EPX_OS_PRIO_COUNT - 1U;
    return (UBaseType_t)(num / den);
}

static void freertos_thread_trampoline(void* arg)
{
    struct epx_os_thread* ctx = (struct epx_os_thread*)arg;
    if (ctx != NULL) {
        ctx->exit_value = ctx->user_entry(ctx->user_arg);
        (void)xSemaphoreGive(ctx->join_sem);
    }
    vTaskDelete(NULL);
}

epx_err_t epx_os_init(void)
{
    return EPX_OK;
}

epx_time_t epx_os_get_time_ms(void)
{
    return freertos_ticks_to_ms(xTaskGetTickCount());
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
    struct epx_os_thread* ctx = (struct epx_os_thread*)pvPortMalloc(sizeof(struct epx_os_thread));
    if (ctx == NULL) {
        return EPX_ERR_NOMEM;
    }
    (void)memset(ctx, 0, sizeof(struct epx_os_thread));
    ctx->user_entry = entry;
    ctx->user_arg = arg;
    ctx->joined = EPX_FALSE;
    ctx->join_sem = xSemaphoreCreateBinary();
    if (ctx->join_sem == NULL) {
        vPortFree(ctx);
        return EPX_ERR_NOMEM;
    }

    uint32_t stack_words;
    if (stack_size == 0U) {
        stack_words = EPX_FREERTOS_STACK_WORDS_MIN;
    } else {
        stack_words = (stack_size + sizeof(StackType_t) - 1U) / (uint32_t)sizeof(StackType_t);
        if (stack_words < EPX_FREERTOS_STACK_WORDS_MIN) {
            stack_words = EPX_FREERTOS_STACK_WORDS_MIN;
        }
    }

    const char* task_name = (name != NULL) ? name : "epx";
    UBaseType_t ux_prio = freertos_map_prio(prio);
#if defined(configSTACK_DEPTH_TYPE)
    configSTACK_DEPTH_TYPE stack_depth = (configSTACK_DEPTH_TYPE)stack_words;
#else
    unsigned short stack_depth = (unsigned short)((stack_words > 0xFFFFu) ? 0xFFFFu : stack_words);
#endif
    BaseType_t ok = xTaskCreate(freertos_thread_trampoline, task_name, stack_depth, ctx, ux_prio, &ctx->task);
    if (ok != pdPASS) {
        vSemaphoreDelete(ctx->join_sem);
        vPortFree(ctx);
        return EPX_ERR_NOMEM;
    }
    *handle = ctx;
    return EPX_OK;
}

epx_err_t epx_os_thread_join(epx_os_thread_handle_t handle, void** result)
{
    if (handle == NULL) {
        return EPX_ERR_PARAM;
    }
    struct epx_os_thread* ctx = (struct epx_os_thread*)handle;
    if (ctx->joined) {
        if (result != NULL) {
            *result = ctx->exit_value;
        }
        return EPX_OK;
    }
    if (xSemaphoreTake(ctx->join_sem, portMAX_DELAY) != pdTRUE) {
        return EPX_ERR;
    }
    ctx->joined = EPX_TRUE;
    if (result != NULL) {
        *result = ctx->exit_value;
    }
    return EPX_OK;
}

void epx_os_delay(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void epx_os_thread_destroy(epx_os_thread_handle_t* handle)
{
    if (handle == NULL || *handle == NULL) {
        return;
    }
    struct epx_os_thread* ctx = (struct epx_os_thread*)*handle;
    if (ctx->join_sem != NULL) {
        vSemaphoreDelete(ctx->join_sem);
    }
    vPortFree(ctx);
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
    (void)printf("[%lu.%03lu] [INFO] ", sec, frac);
    va_list ap;
    va_start(ap, fmt);
    (void)vprintf(fmt, ap);
    va_end(ap);
}
