/**
 * @file epx_topic.h
 * @brief Topic 树与通配符匹配 (+, #). 由 Broker 内部使用.
 */

#ifndef EPX_TOPIC_H
#define EPX_TOPIC_H

#include "epx_config.h"
#include "epx_types.h"
#include "core/epx_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(EPX_CORE_ENABLE)

/** 消息投递到订阅方时调用的回调. */
typedef void (*epx_msg_callback_t)(epx_msg_t msg, void* user_data);

/** 不透明订阅者列表, 由 epx_topic_match 填充. 调用方创建列表, match 填充, 调用方负责销毁. */
struct epx_subscriber_list;
typedef struct epx_subscriber_list epx_subscriber_list_t;

/**
 * @brief 初始化 Topic 树. 由 epx_broker_init 调用.
 * @return 成功返回 EPX_OK.
 */
epx_err_t epx_topic_init(void);

/**
 * @brief 反初始化 Topic 树, 释放所有节点与订阅者. 由 epx_broker_deinit 调用.
 */
void epx_topic_deinit(void);

/**
 * @brief 按过滤器订阅 (可含 + 与 #), 在叶子节点增加 (callback, user_data).
 *        + 匹配一层; # 匹配剩余层级. 仅允许末尾 # (如 "a/#"), 中间 # (如 "a/#/b") 返回 EPX_ERR_PARAM.
 * @param topic_filter  过滤字符串 (如 "sensor/+/temp", "sensor/#"). 最大深度 EPX_TOPIC_MAX_DEPTH.
 * @param callback      匹配到消息时调用.
 * @param user_data     传入回调的用户数据.
 * @return 成功返回 EPX_OK.
 */
epx_err_t epx_topic_subscribe(const char* topic_filter, epx_msg_callback_t callback, void* user_data);

/**
 * @brief 创建空订阅者列表. 必须由 epx_subscriber_list_destroy 销毁.
 * @param out_list  输出列表指针 (不得为 NULL).
 * @return 成功返回 EPX_OK.
 */
epx_err_t epx_subscriber_list_create(epx_subscriber_list_t** out_list);

/**
 * @brief 销毁列表并释放内存.
 * @param list  由 epx_subscriber_list_create 得到的列表.
 */
void epx_subscriber_list_destroy(epx_subscriber_list_t* list);

/**
 * @brief 将发布 topic (不含通配符) 与树匹配, 向列表填入 (callback, user_data). 会先清空列表.
 * @param topic    发布 topic 字符串 (如 "sensor/kitchen/temp").
 * @param out_list 待填充列表 (不得为 NULL, 由 epx_subscriber_list_create 创建).
 * @return 成功返回 EPX_OK, 构建匹配列表失败返回 EPX_ERR_NOMEM.
 */
epx_err_t epx_topic_match(const char* topic, epx_subscriber_list_t* out_list);

/**
 * @brief 列表中订阅者数量.
 * @param list  由 epx_topic_match 得到的列表.
 * @return 数量.
 */
uint32_t epx_subscriber_list_count(epx_subscriber_list_t* list);

/**
 * @brief 按下标取订阅者. 列表销毁后不得再调用.
 * @param list       由 epx_topic_match 得到的列表.
 * @param index      从 0 开始的下标.
 * @param out_cb     输出回调 (可为 NULL).
 * @param out_ud     输出 user_data (可为 NULL).
 * @return 成功返回 EPX_OK, 下标越界返回 EPX_ERR_PARAM.
 */
epx_err_t epx_subscriber_list_get(epx_subscriber_list_t* list, uint32_t index,
                                  epx_msg_callback_t* out_cb, void** out_ud);

#endif /* EPX_CORE_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_TOPIC_H */
