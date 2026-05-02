/**
 * @file epx_rpc.h
 * @brief 基于 Pub/Sub 的 RPC 封装: 请求-响应, 带超时, 防止客户端永久阻塞.
 */

#ifndef EPX_RPC_H
#define EPX_RPC_H

#include "epx_config.h"
#include "epx_types.h"
#include "core/epx_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(EPX_FRAMEWORK_ENABLE) && defined(EPX_RPC_ENABLE)

#define EPX_RPC_MAX_SERVICES        16

/** RPC 请求头: 放在请求 payload 前部, 含 request_id 与 reply_topic 字符串前缀 (配合 request_id 可生成唯一 topic). 紧凑布局, 便于跨设备/线路格式一致. */
#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif
typedef struct epx_rpc_header_t {
    uint32_t request_id;
    char reply_topic[EPX_TOPIC_STR_MAX_LEN];  /* 回复用 topic 字符串 */
} EPX_PACKED epx_rpc_header_t;
#if defined(_MSC_VER)
#pragma pack(pop)
#endif

/** RPC 服务端回调: 收到请求后由框架调用, 回调内应调用 epx_rpc_reply, 返回后框架会 epx_msg_release(req_msg). */
typedef void (*epx_rpc_handler_t)(void* user_data, epx_msg_t req_msg);

/**
 * @brief 发起 RPC 调用: 订阅临时 reply topic, 发请求, 阻塞等待响应或超时.
 * @param target_topic 目标 topic 字符串.
 * @param req_data 请求数据指针.
 * @param req_len 请求长度.
 * @param resp_buf 响应拷贝目标缓冲区.
 * @param resp_buf_len 响应缓冲区大小.
 * @param timeout_ms 超时毫秒数, 超时返回 EPX_ERR_TIMEOUT 并做订阅/资源清理 (安全考虑: 禁止永久阻塞).
 * @return EPX_OK 成功收到响应, EPX_ERR_TIMEOUT 超时, 其它错误码.
 */
epx_err_t epx_rpc_call(const char* target_topic,
                       const void* req_data,
                       size_t req_len,
                       void* resp_buf,
                       size_t resp_buf_len,
                       uint32_t timeout_ms);

/**
 * @brief 回复 RPC 请求: 从 req_msg 解析 header, 向 reply_topic 发布 resp_data.
 * @param req_msg 请求消息 (payload 前部为 epx_rpc_header_t + 用户数据). 调用方负责在适当时机 release.
 * @param resp_data 响应数据.
 * @param resp_len 响应长度.
 * @return EPX_OK 成功.
 */
epx_err_t epx_rpc_reply(epx_msg_t req_msg, const void* resp_data, size_t resp_len);

/**
 * @brief 注册 RPC 服务: 框架订阅 service_method_topic, 收到请求后调用 callback(user_data, req_msg). 每服务一线程一队列.
 * @param service_method_topic  服务方法 topic 字符串.
 * @param callback                请求到达时的处理回调.
 * @param user_data               传入回调的用户数据.
 * @return 成功返回 EPX_OK.
 */
epx_err_t epx_rpc_register(const char* service_method_topic, epx_rpc_handler_t callback, void* user_data);

#endif /* EPX_FRAMEWORK_ENABLE && EPX_RPC_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_RPC_H */
