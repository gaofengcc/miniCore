/**
 * @file epx_broker.h
 * @brief 进程内 Pub/Sub Broker: 按 topic 字符串订阅, 以零拷贝消息指针发布.
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
 * @brief 初始化 Broker 单例 (topic 表, topic 树, 订阅). 仅调用一次.
 * @return 成功返回 EPX_OK.
 */
epx_err_t epx_broker_init(void);

/**
 * @brief 反初始化 Broker, 释放 topic 树与订阅项. 在系统退出时调用.
 */
void epx_broker_deinit(void);

/**
 * @brief 按 topic 过滤器订阅 (可含 + 与 #). 匹配到消息时调用回调.
 * @param topic_filter  过滤器 (如 "sensor/+/temp", "sensor/#").
 * @param callback      匹配时以 (msg, user_data) 调用.
 * @param user_data     传入回调的用户数据.
 * @return 成功返回 EPX_OK.
 */
epx_err_t epx_subscribe(const char* topic_filter, epx_msg_callback_t callback, void* user_data);

/**
 * @brief 发布消息. 按 msg->topic 匹配, 对每个订阅者 retain, 调用回调, 再释放发布者引用.
 * @param msg  已设置 topic 的消息 (如来自 epx_msg_new). 调用方转移所有权, 调用后勿再 release.
 * @return 成功返回 EPX_OK.
 */
epx_err_t epx_publish(epx_msg_t msg);

/**
 * @brief 订阅 topic. 若 topic 未注册则注册, 将 recv_queue 加入订阅者列表.
 *        recv_queue 创建时 msg_len 须为 sizeof(epx_msg_t*) (指针大小).
 * @param topic_str   topic 字符串 (如 "cmd/led").
 * @param recv_queue  接收消息指针 (epx_msg_t*) 的队列. 队列由调用方拥有.
 * @return 成功返回 EPX_OK.
 */
epx_err_t epx_sub(const char* topic_str, epx_os_queue_t recv_queue);

/**
 * @brief 从 topic 取消订阅 recv_queue. 移除订阅并释放内部项.
 * @param topic_str   topic 字符串.
 * @param recv_queue  曾传入 epx_sub 的队列.
 * @return 成功返回 EPX_OK, topic 未找到或队列不在列表中返回 EPX_ERR.
 */
epx_err_t epx_unsub(const char* topic_str, epx_os_queue_t recv_queue);

/**
 * @brief 向 topic 发布. 分配 msg, 拷贝数据, 向各订阅者发送指针, 再释放发布者引用.
 *        同时投递到通配符 topic 树 (epx_subscribe), 便于网关等接收. 若 topic 不在名称表,
 *        仅走 topic 树路径. 每订阅者队列: 非阻塞发送; 若满则该订阅者丢弃,
 *        除非更严重错误, 返回 EPX_ERR_QUEUE_FULL.
 * @param topic_str  topic 字符串.
 * @param data       负载 (拷贝进消息块).
 * @param len        负载长度, 字节.
 * @return 成功返回 EPX_OK. 至少一个接收队列满 (该订阅丢弃) 返回 EPX_ERR_QUEUE_FULL.
 *         订阅者超过 EPX_MAX_SUBSCRIBERS_PER_TOPIC 时多出的未发送返回 EPX_ERR_BUSY
 *         (可选 EPX_BROKER_HOOK_SUBSCRIBER_TRUNCATED). 未初始化或析构中返回 EPX_ERR.
 */
epx_err_t epx_pub(const char* topic_str, const void* data, size_t len);

/**
 * @brief 查询 Broker 是否已初始化. Log 等模块在发布前检查, 未就绪时仅走 Console.
 * @return 1 已就绪, 0 未初始化.
 */
epx_bool_t epx_broker_is_ready(void);

/**
 * @brief 查询 Broker 是否处于空闲 (当前无 epx_pub 路径在执行). 尽力检测, 不替代显式同步;
 *        低功耗等关键场景应在业务层先停发布/订阅任务再依赖该结果.
 * @return 1 空闲, 0 忙.
 */
epx_bool_t epx_broker_is_idle(void);

#endif /* EPX_CORE_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_BROKER_H */
