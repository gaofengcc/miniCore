/**
 * @file epx_topic.h
 * @brief Topic tree and wildcard match (+, #). Internal use by broker.
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

/** Callback invoked when a message is delivered. */
typedef void (*epx_msg_callback_t)(epx_msg_t msg, void* user_data);

/** Opaque subscriber list filled by epx_topic_match. Caller creates, match fills, caller destroys. */
struct epx_subscriber_list;
typedef struct epx_subscriber_list epx_subscriber_list_t;

/**
 * @brief Initialize topic tree. Called by epx_broker_init.
 * @return EPX_OK on success.
 */
epx_err_t epx_topic_init(void);

/**
 * @brief Deinitialize topic tree, free all nodes and subscribers. Called by epx_broker_deinit.
 */
void epx_topic_deinit(void);

/**
 * @brief Subscribe with filter (may contain + and #). Adds (callback, user_data) to tree leaf.
 *        + matches one segment; # matches remaining levels. Only trailing # allowed (e.g. "a/#"),
 *        middle # (e.g. "a/#/b") returns EPX_ERR_PARAM.
 * @param topic_filter  Filter string (e.g. "sensor/+/temp", "sensor/#"). Max depth EPX_TOPIC_MAX_DEPTH.
 * @param callback      Called on matching message.
 * @param user_data     Passed to callback.
 * @return EPX_OK on success.
 */
epx_err_t epx_topic_subscribe(const char* topic_filter, epx_msg_callback_t callback, void* user_data);

/**
 * @brief Create empty subscriber list. Must be destroyed by epx_subscriber_list_destroy.
 * @param out_list  Output list (must not be NULL).
 * @return EPX_OK on success.
 */
epx_err_t epx_subscriber_list_create(epx_subscriber_list_t** out_list);

/**
 * @brief Destroy list and free memory.
 * @param list  List from epx_subscriber_list_create.
 */
void epx_subscriber_list_destroy(epx_subscriber_list_t* list);

/**
 * @brief Match topic (no wildcards) against tree, fill list with (callback, user_data). Clears list first.
 * @param topic    Publish topic string (e.g. "sensor/kitchen/temp").
 * @param out_list List to fill (must not be NULL, created by epx_subscriber_list_create).
 * @return EPX_OK on success, EPX_ERR_NOMEM if building the match list fails.
 */
epx_err_t epx_topic_match(const char* topic, epx_subscriber_list_t* out_list);

/**
 * @brief Number of subscribers in list.
 * @param list  List from epx_topic_match.
 * @return Count.
 */
uint32_t epx_subscriber_list_count(epx_subscriber_list_t* list);

/**
 * @brief Get subscriber at index. Do not call after list destroyed.
 * @param list       List from epx_topic_match.
 * @param index      Zero-based index.
 * @param out_cb     Output callback (may be NULL).
 * @param out_ud     Output user_data (may be NULL).
 * @return EPX_OK on success, EPX_ERR_PARAM if index out of range.
 */
epx_err_t epx_subscriber_list_get(epx_subscriber_list_t* list, uint32_t index,
                                  epx_msg_callback_t* out_cb, void** out_ud);

#endif /* EPX_CORE_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_TOPIC_H */
