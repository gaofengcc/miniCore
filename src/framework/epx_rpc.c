/**
 * @file epx_rpc.c
 * @brief 基于 Pub/Sub 的 RPC: 请求-响应, 带超时, 超时后退订并销毁队列 (防止 Client 永久阻塞).
 */
#define EPX_LOG_TAG "RPC"
#include "framework/epx_rpc.h"
#include "framework/epx_topic_names.h"
#include "epx_config.h"

/* RPC Log 可选依赖: 默认关闭以保持轻量核心独立性 */
#ifndef EPX_RPC_ENABLE_LOG
    #define EPX_RPC_ENABLE_LOG 0
#endif

#if EPX_RPC_ENABLE_LOG
    #include "services/epx_log.h"
    #define RPC_LOG_INFO(...)  EPX_LOG_INFO(__VA_ARGS__)
#else
    #define RPC_LOG_INFO(...)  ((void)0)
#endif
#if defined(EPX_FRAMEWORK_ENABLE) && defined(EPX_RPC_ENABLE)

#include "core/epx_broker.h"
#include "core/epx_msg.h"
#include "epx_types.h"
#include "osal/epx_os_mem.h"
#include "osal/epx_os_queue.h"
#include "osal/epx_os_thread.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define EPX_RPC_QUEUE_DEPTH         4
#define EPX_RPC_MAX_SERVICES        16
#define EPX_RPC_SERVER_QUEUE_DEPTH  8

static uint32_t g_next_request_id = 0;

epx_err_t epx_rpc_call(const char* target_topic,
                       const void* req_data,
                       size_t req_len,
                       void* resp_buf,
                       size_t resp_buf_len,
                       uint32_t timeout_ms)
{
    if (target_topic == NULL || resp_buf == NULL || resp_buf_len == 0) {
        return EPX_ERR_PARAM;
    }

    uint32_t req_id = (uint32_t)(__sync_fetch_and_add((int*)&g_next_request_id, 1) & 0x7FFFFFFFu);
    /* 回复 topic 约定: service_name/method_name/reply/{req_id} */
    char reply_topic[EPX_TOPIC_STR_MAX_LEN];
    int n = snprintf(reply_topic, sizeof(reply_topic), "%s%s%" PRIu32, target_topic, EPX_TOPIC_RPC_REPLY_SUFFIX, req_id);
    if (n < 0 || (size_t)n >= sizeof(reply_topic)) {
        return EPX_ERR_PARAM;
    }

    epx_os_queue_t recv_queue = NULL;
    epx_err_t ret = epx_os_queue_create(&recv_queue, sizeof(epx_msg_t), EPX_RPC_QUEUE_DEPTH);
    if (ret != EPX_OK) {
        return ret;
    }

    ret = epx_subscribe_queue(reply_topic, recv_queue);
    if (ret != EPX_OK) {
        epx_os_queue_destroy(&recv_queue);
        return ret;
    }

    epx_rpc_header_t header;
    header.request_id = req_id;
    (void)memcpy(header.reply_topic, reply_topic, sizeof(header.reply_topic));
    header.reply_topic[EPX_TOPIC_STR_MAX_LEN - 1] = '\0';

    size_t payload_len = sizeof(epx_rpc_header_t) + req_len;
    uint8_t* payload = (uint8_t*)epx_os_malloc(payload_len);
    if (payload == NULL) {
        epx_unsubscribe_queue(reply_topic, recv_queue);
        epx_os_queue_destroy(&recv_queue);
        return EPX_ERR_NOMEM;
    }
    memcpy(payload, &header, sizeof(epx_rpc_header_t));
    if (req_data != NULL && req_len > 0) {
        memcpy(payload + sizeof(epx_rpc_header_t), req_data, req_len);
    }

    ret = epx_publish_data(target_topic, payload, payload_len);
    epx_os_free(payload);
    if (ret != EPX_OK) {
        epx_unsubscribe_queue(reply_topic, recv_queue);
        epx_os_queue_destroy(&recv_queue);
        return ret;
    }

    epx_msg_t msg = NULL;
    ret = epx_os_queue_recv(recv_queue, &msg, timeout_ms);
    /* 安全考虑: 超时必须退订并销毁队列, 禁止永久阻塞. */
    if (ret == EPX_ERR_TIMEOUT) {
        epx_unsubscribe_queue(reply_topic, recv_queue);
        epx_os_queue_destroy(&recv_queue);
        return EPX_ERR_TIMEOUT;
    }
    if (ret != EPX_OK || msg == NULL) {
        epx_unsubscribe_queue(reply_topic, recv_queue);
        epx_os_queue_destroy(&recv_queue);
        return (ret != EPX_OK) ? ret : EPX_ERR;
    }

    size_t resp_size = epx_msg_size(msg);
    size_t copy_len = resp_size < resp_buf_len ? resp_size : resp_buf_len;
    if (copy_len > 0) {
        void* src = epx_msg_payload(msg);
        if (src != NULL) {
            memcpy(resp_buf, src, copy_len);
        }
    }
    if (copy_len < resp_buf_len) {
        memset((char*)resp_buf + copy_len, 0, resp_buf_len - copy_len);
    }
    epx_msg_release(msg);
    epx_unsubscribe_queue(reply_topic, recv_queue);
    epx_os_queue_destroy(&recv_queue);
    return EPX_OK;
}

epx_err_t epx_rpc_reply(epx_msg_t req_msg, const void* resp_data, size_t resp_len)
{
    if (req_msg == NULL) {
        return EPX_ERR_PARAM;
    }
    size_t sz = epx_msg_size(req_msg);
    if (sz < sizeof(epx_rpc_header_t)) {
        return EPX_ERR_PARAM;
    }
    void* p = epx_msg_payload(req_msg);
    if (p == NULL) {
        return EPX_ERR_PARAM;
    }
    epx_rpc_header_t* h = (epx_rpc_header_t*)p;
    RPC_LOG_INFO("RPC reply to topic: %s", h->reply_topic);
    return epx_publish_data(h->reply_topic, resp_data, resp_len);
}

typedef struct {
    char topic[EPX_TOPIC_STR_MAX_LEN];
    epx_os_queue_t queue;
    epx_os_thread_handle_t thread;
    epx_rpc_handler_t callback;
    void* user_data;
    int in_use;
} rpc_service_slot_t;

static rpc_service_slot_t g_rpc_services[EPX_RPC_MAX_SERVICES];

/**
 * @brief RPC 服务端线程入口: 从队列取消息, 调用 slot 回调, 再 release 消息.
 * @param arg  指向 rpc_service_slot_t 的指针.
 * @return 始终为 NULL (当前实现不退出循环).
 */
static void* rpc_server_entry(void* arg)
{
    rpc_service_slot_t* slot = (rpc_service_slot_t*)arg;
    epx_msg_t msg = NULL;
    while (1) {
        epx_err_t ret = epx_os_queue_recv(slot->queue, &msg, EPX_OS_QUEUE_WAIT_FOREVER);
        if (ret == EPX_OK && msg != NULL) {
            if (slot->callback != NULL) {
                slot->callback(slot->user_data, msg);
            }
            epx_msg_release(msg);
            msg = NULL;
        }
    }
    return NULL;
}

epx_err_t epx_rpc_register(const char* service_method_topic, epx_rpc_handler_t callback, void* user_data)
{
    if (service_method_topic == NULL || callback == NULL) {
        return EPX_ERR_PARAM;
    }
    if (strlen(service_method_topic) >= (size_t)EPX_TOPIC_STR_MAX_LEN) {
        return EPX_ERR_PARAM;
    }

    uint32_t i;
    for (i = 0; i < EPX_RPC_MAX_SERVICES; i++) {
        if (!g_rpc_services[i].in_use) {
            break;
        }
    }
    if (i >= EPX_RPC_MAX_SERVICES) {
        return EPX_ERR;
    }

    rpc_service_slot_t* slot = &g_rpc_services[i];
    (void)memcpy(slot->topic, service_method_topic, strlen(service_method_topic) + 1);

    epx_err_t ret = epx_os_queue_create(&slot->queue, sizeof(epx_msg_t), EPX_RPC_SERVER_QUEUE_DEPTH);
    if (ret != EPX_OK) {
        return ret;
    }
    ret = epx_subscribe_queue(slot->topic, slot->queue);
    if (ret != EPX_OK) {
        epx_os_queue_destroy(&slot->queue);
        return ret;
    }
    slot->callback = callback;
    slot->user_data = user_data;
    slot->in_use = 1;

    ret = epx_os_thread_create(&slot->thread, "rpc_srv", 0, EPX_OS_PRIO_NORMAL, rpc_server_entry, slot);
    if (ret != EPX_OK) {
        epx_unsubscribe_queue(slot->topic, slot->queue);
        epx_os_queue_destroy(&slot->queue);
        slot->in_use = 0;
        return ret;
    }
    return EPX_OK;
}

#endif /* EPX_FRAMEWORK_ENABLE && EPX_RPC_ENABLE */
