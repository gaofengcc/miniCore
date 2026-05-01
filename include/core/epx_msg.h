/**
 * @file epx_msg.h
 * @brief Zero-copy message block: ref_count, payload, optional destructor.
 *        Supports memory pool for reduced allocation overhead.
 */

#ifndef EPX_MSG_H
#define EPX_MSG_H

#include "epx_config.h"
#include "epx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(EPX_CORE_ENABLE)

struct epx_msg_block;
typedef struct epx_msg_block* epx_msg_t;

/**
 * @brief Initialize message module (global mutex for ref_count). Call once before alloc/retain/release.
 * @return EPX_OK on success.
 */
epx_err_t epx_msg_init(void);

/**
 * @brief Initialize message module with memory pool for small messages.
 *        The pool will be used for messages with payload_size <= pool_block_size.
 * @param pool_block_size  Block size for memory pool (e.g., 128, 256).
 * @param pool_block_count Number of blocks in pool.
 * @return EPX_OK on success.
 */
epx_err_t epx_msg_init_with_pool(size_t pool_block_size, uint32_t pool_block_count);

/**
 * @brief Allocate message block from pool (if available) or heap.
 *        ref_count = 1. Payload size = size bytes. No topic.
 * @param size  Payload size in bytes.
 * @return Message handle, or NULL on failure.
 */
epx_msg_t epx_msg_alloc(size_t size);

/**
 * @brief Create message with topic and payload. Zero-copy: topic pointer stored directly.
 *        ref_count = 1. Topic string NOT copied (pointer stored), payload_len bytes reserved.
 *        Caller must ensure topic string outlives the message.
 * @param topic         Topic string (pointer stored, NOT copied, max EPX_TOPIC_STR_MAX_LEN).
 *                      Must not be NULL. Must remain valid until message is released.
 * @param payload_len   Payload size in bytes.
 * @return Message handle, or NULL on failure.
 */
epx_msg_t epx_msg_new(const char* topic, size_t payload_len);

/**
 * @brief Create message with topic copied (legacy behavior for backward compatibility).
 * @param topic         Topic string (copied, max EPX_TOPIC_STR_MAX_LEN). Must not be NULL.
 * @param payload_len   Payload size in bytes.
 * @return Message handle, or NULL on failure.
 */
epx_msg_t epx_msg_new_copy(const char* topic, size_t payload_len);

/**
 * @brief Increment ref_count. NULL-safe (no-op if msg is NULL).
 * @param msg  Message handle.
 */
void epx_msg_retain(epx_msg_t msg);

/**
 * @brief Decrement ref_count. If 0, call destructor (if set) and free. NULL-safe.
 * @param msg  Message handle. After release, must not be used again.
 */
void epx_msg_release(epx_msg_t msg);

/**
 * @brief Set destructor called when ref_count reaches 0 (before free). Optional, for testing or custom cleanup.
 * @param msg         Message handle (must not be NULL).
 * @param destructor  Function to call with block pointer, or NULL to clear.
 */
void epx_msg_set_destructor(epx_msg_t msg, void (*destructor)(void*));

/**
 * @brief Get pointer to payload (read-only). Do not use after epx_msg_release.
 * @param msg  Message handle (must not be NULL).
 * @return Pointer to payload (data[]).
 */
void* epx_msg_payload(epx_msg_t msg);

/**
 * @brief Get payload size in bytes.
 * @param msg  Message handle (must not be NULL).
 * @return Payload size.
 */
size_t epx_msg_size(epx_msg_t msg);

/**
 * @brief Get topic string. NULL if message was created with epx_msg_alloc (no topic).
 * @param msg  Message handle (must not be NULL).
 * @return Topic string, or NULL.
 */
const char* epx_msg_topic(epx_msg_t msg);

#endif /* EPX_CORE_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_MSG_H */
