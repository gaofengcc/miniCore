/**
 * @file epx_os_queue.h
 * @brief OSAL message queue with timeout. Core for future Pub/Sub.
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
 * @brief Create a message queue.
 * @param queue    Output queue handle (must not be NULL).
 * @param msg_len  Size of one message in bytes.
 * @param depth    Maximum number of messages in queue.
 * @return EPX_OK on success, EPX_ERR_PARAM if queue is NULL or msg_len/depth is 0, EPX_ERR_NOMEM on failure.
 */
epx_err_t epx_os_queue_create(epx_os_queue_t* queue, size_t msg_len, uint32_t depth);

/**
 * @brief Send a message (copy). Blocks until space available or timeout.
 * @param queue     Queue handle (must not be NULL).
 * @param msg       Message buffer (must not be NULL, size >= queue msg_len).
 * @param timeout_ms  Timeout in ms, or EPX_OS_QUEUE_WAIT_FOREVER.
 * @return EPX_OK on success, EPX_ERR_PARAM if queue/msg is NULL, EPX_ERR_TIMEOUT on timeout.
 */
epx_err_t epx_os_queue_send(epx_os_queue_t queue, const void* msg, uint32_t timeout_ms);

/**
 * @brief Receive a message (copy). Supports timeout.
 * @param queue     Queue handle (must not be NULL).
 * @param msg       Output buffer (must not be NULL, size >= queue msg_len).
 * @param timeout_ms  Timeout in ms, or EPX_OS_QUEUE_WAIT_FOREVER.
 * @return EPX_OK on success, EPX_ERR_PARAM if queue/msg is NULL, EPX_ERR_TIMEOUT on timeout.
 */
epx_err_t epx_os_queue_recv(epx_os_queue_t queue, void* msg, uint32_t timeout_ms);

/**
 * @brief Destroy queue. No-op if queue is NULL.
 * @param queue  Queue handle.
 */
void epx_os_queue_destroy(epx_os_queue_t* queue);

#endif /* EPX_OSAL_ENABLE && EPX_OS_QUEUE_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_OS_QUEUE_H */
