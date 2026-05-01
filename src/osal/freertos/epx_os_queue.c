/**
 * @file epx_os_queue.c
 * @brief FreeRTOS queue (xQueueCreate) with timeout semantics.
 */

#include "osal/epx_os_queue.h"

#include "FreeRTOS.h"
#include "queue.h"

#include <string.h>

epx_err_t epx_os_queue_create(epx_os_queue_t* queue, size_t msg_len, uint32_t depth)
{
    if (queue == NULL || msg_len == 0 || depth == 0) {
        return EPX_ERR_PARAM;
    }
    QueueHandle_t qh = xQueueCreate((UBaseType_t)depth, (UBaseType_t)msg_len);
    if (qh == NULL) {
        return EPX_ERR_NOMEM;
    }
    *queue = (epx_os_queue_t)qh;
    return EPX_OK;
}

epx_err_t epx_os_queue_send(epx_os_queue_t queue, const void* msg, uint32_t timeout_ms)
{
    if (queue == NULL || msg == NULL) {
        return EPX_ERR_PARAM;
    }
    QueueHandle_t qh = (QueueHandle_t)queue;
    TickType_t ticks;
    if (timeout_ms == EPX_OS_QUEUE_WAIT_FOREVER) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeout_ms);
    }
    if (xQueueSend(qh, msg, ticks) == pdTRUE) {
        return EPX_OK;
    }
    return EPX_ERR_TIMEOUT;
}

epx_err_t epx_os_queue_recv(epx_os_queue_t queue, void* msg, uint32_t timeout_ms)
{
    if (queue == NULL || msg == NULL) {
        return EPX_ERR_PARAM;
    }
    QueueHandle_t qh = (QueueHandle_t)queue;
    TickType_t ticks;
    if (timeout_ms == EPX_OS_QUEUE_WAIT_FOREVER) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeout_ms);
    }
    if (xQueueReceive(qh, msg, ticks) == pdTRUE) {
        return EPX_OK;
    }
    return EPX_ERR_TIMEOUT;
}

void epx_os_queue_destroy(epx_os_queue_t* queue)
{
    if (queue == NULL || *queue == NULL) {
        return;
    }
    QueueHandle_t qh = (QueueHandle_t)*queue;
    vQueueDelete(qh);
    *queue = NULL;
}
