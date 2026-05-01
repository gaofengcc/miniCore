/**
 * @file epx_broker.h
 * @brief In-process Pub/Sub broker: subscribe by topic string, publish with zero-copy msg pointer.
 */

#ifndef EPX_BROKER_H
#define EPX_BROKER_H

#include "epx_config.h"
#include "epx_types.h"
#include "core/epx_msg.h"
#include "core/epx_topic.h"
#include "osal/epx_os_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(EPX_CORE_ENABLE)

/**
 * @brief Initialize broker singleton (topic table, topic tree, subscription). Call once.
 * @return EPX_OK on success.
 */
epx_err_t epx_broker_init(void);

/**
 * @brief Deinitialize broker, free topic tree and subscription entries. Call when tearing down.
 */
void epx_broker_deinit(void);

/**
 * @brief Subscribe by topic filter (may contain + and #). Callback invoked on matching message.
 * @param topic_filter  Filter (e.g. "sensor/+/temp", "sensor/#").
 * @param callback      Called with (msg, user_data) on match.
 * @param user_data     Passed to callback.
 * @return EPX_OK on success.
 */
epx_err_t epx_subscribe(const char* topic_filter, epx_msg_callback_t callback, void* user_data);

/**
 * @brief Publish message. Match msg->topic, retain per subscriber, invoke callback, then release publisher ref.
 * @param msg  Message with topic set (e.g. from epx_msg_new). Caller transfers ownership; no release after call.
 * @return EPX_OK on success.
 */
epx_err_t epx_publish(epx_msg_t msg);

/**
 * @brief Subscribe to topic. Register topic if new, add recv_queue to subscriber list.
 *        recv_queue must be created with msg_len = sizeof(epx_msg_t*) (pointer size).
 * @param topic_str   Topic string (e.g. "cmd/led").
 * @param recv_queue  Queue to receive message pointers (epx_msg_t*). Caller owns queue.
 * @return EPX_OK on success.
 */
epx_err_t epx_sub(const char* topic_str, epx_os_queue_t recv_queue);

/**
 * @brief Unsubscribe recv_queue from topic. Removes the subscription, frees internal entry.
 * @param topic_str   Topic string.
 * @param recv_queue  Queue that was previously passed to epx_sub.
 * @return EPX_OK on success, EPX_ERR if topic not found or queue not in subscriber list.
 */
epx_err_t epx_unsub(const char* topic_str, epx_os_queue_t recv_queue);

/**
 * @brief Publish to topic. Alloc msg, copy data, send pointer to each subscriber, then release publisher ref.
 *        If topic has no subscribers, returns without allocating. Queue full: drop and release for that sub.
 * @param topic_str  Topic string.
 * @param data       Payload (copied into message block).
 * @param len        Payload length in bytes.
 * @return EPX_OK on success, EPX_ERR if topic not found (no subscribers),
 *         EPX_ERR_QUEUE_FULL if at least one subscriber queue was full (message dropped for that sub).
 */
epx_err_t epx_pub(const char* topic_str, const void* data, size_t len);

/**
 * @brief 查询 Broker 是否已初始化. Log 等模块在发布前检查, 未就绪时仅走 Console.
 * @return 1 已就绪, 0 未初始化.
 */
epx_bool_t epx_broker_is_ready(void);

/**
 * @brief 查询 Broker 是否处于空闲 (当前无 epx_pub 在执行). 供 PM 模块空闲检测使用.
 * @return 1 空闲, 0 忙.
 */
epx_bool_t epx_broker_is_idle(void);

#endif /* EPX_CORE_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_BROKER_H */
