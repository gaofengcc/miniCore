/**
 * @file epx_os_queue.h
 * @brief OSAL 带超时的消息队列. 为后续 Pub/Sub 的核心依赖.
 */

#ifndef EPX_OS_QUEUE_H
#define EPX_OS_QUEUE_H

#include "epx_config.h"
#include "epx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(EPX_OSAL_ENABLE) && defined(EPX_OS_QUEUE_ENABLE)

#define EPX_OS_QUEUE_WAIT_FOREVER  ((uint32_t)-1)

struct epx_os_queue;
typedef struct epx_os_queue* epx_os_queue_t;

/**
 * @brief 创建消息队列.
 * @param queue    输出队列句柄 (不得为 NULL).
 * @param msg_len  单条消息字节数.
 * @param depth    队列最大消息条数.
 * @return 成功返回 EPX_OK, queue 为 NULL 或 msg_len/depth 为 0 返回 EPX_ERR_PARAM, 失败返回 EPX_ERR_NOMEM.
 */
epx_err_t epx_os_queue_create(epx_os_queue_t* queue, size_t msg_len, uint32_t depth);

/**
 * @brief 发送消息 (拷贝). 阻塞直到有空间或超时.
 * @param queue       队列句柄 (不得为 NULL).
 * @param msg         消息缓冲区 (不得为 NULL, 大小 >= 队列 msg_len).
 * @param timeout_ms  超时毫秒, 或 EPX_OS_QUEUE_WAIT_FOREVER.
 * @return 成功返回 EPX_OK, queue/msg 为 NULL 返回 EPX_ERR_PARAM, 超时返回 EPX_ERR_TIMEOUT.
 */
epx_err_t epx_os_queue_send(epx_os_queue_t queue, const void* msg, uint32_t timeout_ms);

/**
 * @brief 接收消息 (拷贝). 支持超时.
 * @param queue       队列句柄 (不得为 NULL).
 * @param msg         输出缓冲区 (不得为 NULL, 大小 >= 队列 msg_len).
 * @param timeout_ms  超时毫秒, 或 EPX_OS_QUEUE_WAIT_FOREVER.
 * @return 成功返回 EPX_OK, queue/msg 为 NULL 返回 EPX_ERR_PARAM, 超时返回 EPX_ERR_TIMEOUT.
 */
epx_err_t epx_os_queue_recv(epx_os_queue_t queue, void* msg, uint32_t timeout_ms);

/**
 * @brief 销毁队列. queue 为 NULL 时为空操作.
 * @param queue  队列句柄.
 */
void epx_os_queue_destroy(epx_os_queue_t* queue);

#endif /* EPX_OSAL_ENABLE && EPX_OS_QUEUE_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_OS_QUEUE_H */
