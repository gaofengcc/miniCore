/**
 * @file epx_broker.c
 * @brief Broker: topic 注册表, 订阅表, 以零拷贝指针发布. 队列满时丢弃.
 */

#include "core/epx_broker.h"
#include "core/epx_topic.h"
#include "core/epx_topic_tbl_priv.h"
#include "epx_config.h"
#include "osal/epx_os_mem.h"
#include "osal/epx_os_mutex.h"
#include "osal/epx_os_queue.h"

#if defined(EPX_CORE_ENABLE)

#include <stdlib.h>
#include <string.h>

#ifndef EPX_BROKER_LOG_DROP
#define EPX_BROKER_LOG_DROP(ptr)  ((void)(ptr))
#endif

typedef struct sub_entry {
    epx_os_queue_t subscriber_queue;
    void* context;
    struct sub_entry* next;
} sub_entry_t;

static sub_entry_t* g_subs[EPX_MAX_TOPICS];
static epx_os_mutex_t g_broker_mutex = NULL;
static epx_bool_t g_inited = 0;
static volatile int g_broker_teardown = 0;
static volatile int g_in_pub = 0;

epx_err_t epx_broker_init(void)
{
    if (g_inited) {
        return EPX_OK;
    }
    g_broker_teardown = 0;
    epx_err_t ret = epx_msg_init();
    if (ret != EPX_OK) {
        return ret;
    }
    ret = epx_topic_tbl_init();
    if (ret != EPX_OK) {
        return ret;
    }
    ret = epx_topic_init();
    if (ret != EPX_OK) {
        return ret;
    }
    ret = epx_os_mutex_create(&g_broker_mutex);
    if (ret != EPX_OK) {
        return ret;
    }
    memset(g_subs, 0, sizeof(g_subs));
    g_inited = 1;
    return EPX_OK;
}

/**
 * @brief 反初始化 Broker: 释放订阅项, 销毁 topic 树与互斥锁.
 *        调用后, 在再次 epx_broker_init 之前 epx_subscribe_queue/epx_publish_data/epx_subscribe/epx_publish 将返回 EPX_ERR.
 */
void epx_broker_deinit(void)
{
    if (!g_inited) {
        return;
    }
    g_broker_teardown = 1;
    if (g_broker_mutex != NULL) {
        epx_os_mutex_lock(g_broker_mutex);
    }
    for (uint32_t i = 0; i < (uint32_t)EPX_MAX_TOPICS; i++) {
        sub_entry_t* p = g_subs[i];
        while (p != NULL) {
            sub_entry_t* next = p->next;
            epx_os_free(p);
            p = next;
        }
        g_subs[i] = NULL;
    }
    if (g_broker_mutex != NULL) {
        epx_os_mutex_unlock(g_broker_mutex);
        epx_os_mutex_destroy(&g_broker_mutex);
    }
    epx_topic_deinit();
    g_inited = 0;
    g_broker_teardown = 0;
}

epx_err_t epx_subscribe_queue(const char* topic_str, epx_os_queue_t recv_queue)
{
    if (topic_str == NULL || recv_queue == NULL) {
        return EPX_ERR_PARAM;
    }
    if (g_broker_teardown) {
        return EPX_ERR;
    }
    if (!g_inited || g_broker_mutex == NULL) {
        return EPX_ERR;
    }
    uint16_t id = 0;
    epx_err_t ret = epx_topic_tbl_register(topic_str, &id);
    if (ret != EPX_OK || id >= (uint16_t)EPX_MAX_TOPICS) {
        return ret;
    }
    sub_entry_t* entry = (sub_entry_t*)epx_os_malloc(sizeof(sub_entry_t));
    if (entry == NULL) {
        return EPX_ERR_NOMEM;
    }
    entry->subscriber_queue = recv_queue;
    entry->context = NULL;
    entry->next = NULL;

    epx_os_mutex_lock(g_broker_mutex);
    entry->next = g_subs[id];
    g_subs[id] = entry;
    epx_os_mutex_unlock(g_broker_mutex);
    return EPX_OK;
}

epx_err_t epx_unsubscribe_queue(const char* topic_str, epx_os_queue_t recv_queue)
{
    if (topic_str == NULL || recv_queue == NULL) {
        return EPX_ERR_PARAM;
    }
    if (g_broker_teardown) {
        return EPX_ERR;
    }
    if (!g_inited || g_broker_mutex == NULL) {
        return EPX_ERR;
    }
    uint16_t id = 0;
    epx_err_t ret = epx_topic_tbl_lookup(topic_str, &id);
    if (ret != EPX_OK || id >= (uint16_t)EPX_MAX_TOPICS) {
        return EPX_ERR;
    }
    epx_os_mutex_lock(g_broker_mutex);
    sub_entry_t* prev = NULL;
    for (sub_entry_t* p = g_subs[id]; p != NULL; prev = p, p = p->next) {
        if (p->subscriber_queue == recv_queue) {
            if (prev != NULL) {
                prev->next = p->next;
            } else {
                g_subs[id] = p->next;
            }
            epx_os_mutex_unlock(g_broker_mutex);
            epx_os_free(p);
            return EPX_OK;
        }
    }
    epx_os_mutex_unlock(g_broker_mutex);
    return EPX_ERR;
}

epx_bool_t epx_broker_is_ready(void)
{
    return g_inited;
}

epx_bool_t epx_broker_is_idle(void)
{
    return (epx_bool_t)(g_in_pub == 0);
}

/* 将相同负载投递到 topic 树 (epx_subscribe 回调), 便于网关等接收. */
static void publish_data_to_topic_tree(const char* topic_str, const void* data, size_t len)
{
    epx_msg_t m = epx_msg_new(topic_str, len);
    if (m == NULL) {
        return;
    }
    if (len > 0 && data != NULL) {
        void* pl = epx_msg_payload(m);
        if (pl != NULL) {
            memcpy(pl, data, len);
        }
    }
    (void)epx_publish(m);
}

epx_err_t epx_publish_data(const char* topic_str, const void* data, size_t len)
{
    if (topic_str == NULL || topic_str[0] == '\0') {
        return EPX_ERR_PARAM;
    }
    if (g_broker_teardown) {
        return EPX_ERR;
    }
    if (!g_inited || g_broker_mutex == NULL) {
        return EPX_ERR;
    }
    uint16_t id = 0;
    epx_err_t ret = epx_topic_tbl_lookup(topic_str, &id);
    if (ret != EPX_OK || id >= (uint16_t)EPX_MAX_TOPICS) {
        /* 不在表中的 topic (如 reply): 仅投递 topic 树以便网关等接收. */
        publish_data_to_topic_tree(topic_str, data, len);
        return EPX_OK;
    }

    g_in_pub = 1;
    epx_os_mutex_lock(g_broker_mutex);
    uint32_t total_subs = 0;
    for (sub_entry_t* p = g_subs[id]; p != NULL; p = p->next) {
        total_subs++;
    }
    epx_bool_t truncated = (total_subs > (uint32_t)EPX_MAX_SUBSCRIBERS_PER_TOPIC);
    if (truncated) {
        EPX_BROKER_HOOK_SUBSCRIBER_TRUNCATED(topic_str,
                                             total_subs - (uint32_t)EPX_MAX_SUBSCRIBERS_PER_TOPIC);
    }
    epx_os_queue_t queues[EPX_MAX_SUBSCRIBERS_PER_TOPIC];
    uint32_t n = 0;
    for (sub_entry_t* p = g_subs[id]; p != NULL && n < (uint32_t)EPX_MAX_SUBSCRIBERS_PER_TOPIC; p = p->next) {
        queues[n++] = p->subscriber_queue;
    }
    epx_os_mutex_unlock(g_broker_mutex);

    if (n > 0) {
        epx_msg_t msg = epx_msg_alloc(len);
        if (msg != NULL) {
            void* payload = epx_msg_payload(msg);
            if (data != NULL && len > 0) {
                memcpy(payload, data, len);
            }
            epx_bool_t any_drop = EPX_FALSE;
            for (uint32_t i = 0; i < n; i++) {
                epx_msg_retain(msg);
                ret = epx_os_queue_send(queues[i], &msg, 0);
                if (ret != EPX_OK) {
                    epx_msg_release(msg);
                    EPX_BROKER_LOG_DROP(msg);
                    any_drop = EPX_TRUE;
                }
            }
            epx_msg_release(msg);
            g_in_pub = 0;
            if (any_drop) {
                return EPX_ERR_QUEUE_FULL;
            }
            if (truncated) {
                return EPX_ERR_BUSY;
            }
            return EPX_OK;
        }
    }
    /* 同时投递 topic 树 (如网关订 #), 便于 WS 客户端收到回复. */
    publish_data_to_topic_tree(topic_str, data, len);
    g_in_pub = 0;
    if (truncated) {
        return EPX_ERR_BUSY;
    }
    return EPX_OK;
}

epx_err_t epx_subscribe(const char* topic_filter, epx_msg_callback_t callback, void* user_data)
{
    if (topic_filter == NULL || callback == NULL) {
        return EPX_ERR_PARAM;
    }
    if (g_broker_teardown) {
        return EPX_ERR;
    }
    if (!g_inited) {
        return EPX_ERR;
    }
    return epx_topic_subscribe(topic_filter, callback, user_data);
}

epx_err_t epx_publish(epx_msg_t msg)
{
    if (msg == NULL) {
        return EPX_ERR_PARAM;
    }
    if (g_broker_teardown) {
        epx_msg_release(msg);
        return EPX_ERR;
    }
    if (!g_inited) {
        epx_msg_release(msg);
        return EPX_ERR;
    }
    const char* topic = epx_msg_topic(msg);
    if (topic == NULL || topic[0] == '\0') {
        epx_msg_release(msg);
        return EPX_ERR_PARAM;
    }
    epx_subscriber_list_t* list = NULL;
    epx_err_t ret = epx_subscriber_list_create(&list);
    if (ret != EPX_OK || list == NULL) {
        epx_msg_release(msg);
        return (ret == EPX_OK) ? EPX_ERR_NOMEM : ret;
    }
    ret = epx_topic_match(topic, list);
    if (ret != EPX_OK) {
        epx_subscriber_list_destroy(list);
        epx_msg_release(msg);
        return ret;
    }
    uint32_t n = epx_subscriber_list_count(list);
    if (n == 0) {
        epx_subscriber_list_destroy(list);
        epx_msg_release(msg);
        return EPX_OK;
    }
    g_in_pub = 1;
    for (uint32_t i = 0; i < n; i++) {
        epx_msg_callback_t cb = NULL;
        void* ud = NULL;
        epx_subscriber_list_get(list, i, &cb, &ud);
        if (cb != NULL) {
            epx_msg_retain(msg);
            cb(msg, ud);
        }
    }
    epx_msg_release(msg);
    epx_subscriber_list_destroy(list);
    g_in_pub = 0;
    return EPX_OK;
}

#endif /* EPX_CORE_ENABLE */
