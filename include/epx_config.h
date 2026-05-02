/**
 * @file epx_config.h
 * @brief miniCore 精简配置. 仅保留 OSAL, Broker 和可选 RPC 所需开关.
 */

#ifndef EPX_CONFIG_H
#define EPX_CONFIG_H

/* OSAL modules */
#define EPX_OSAL_ENABLE           1
#define EPX_OS_THREAD_ENABLE      1
#define EPX_OS_MUTEX_ENABLE       1
#define EPX_OS_QUEUE_ENABLE       1
#define EPX_OS_MEM_ENABLE         1

/* Core */
#define EPX_CORE_ENABLE           1
#define EPX_MAX_TOPICS            64
#define EPX_MAX_SUBSCRIBERS_PER_TOPIC  32
#define EPX_TOPIC_STR_MAX_LEN     64
#define EPX_TOPIC_MAX_DEPTH       8

/* Optional framework features. */
#ifndef EPX_FRAMEWORK_ENABLE
#define EPX_FRAMEWORK_ENABLE      0
#endif

#ifndef EPX_RPC_ENABLE
#define EPX_RPC_ENABLE            0
#endif

/* Debug hooks for external memory tracing. */
#ifndef EPX_OS_MEM_DEBUG_HOOK_ALLOC
#define EPX_OS_MEM_DEBUG_HOOK_ALLOC(ptr, size)   ((void)0)
#endif

#ifndef EPX_OS_MEM_DEBUG_HOOK_FREE
#define EPX_OS_MEM_DEBUG_HOOK_FREE(ptr)          ((void)0)
#endif

/* Optional: epx_publish_data truncated subscriber list (dropped = subs beyond EPX_MAX_SUBSCRIBERS_PER_TOPIC). */
#ifndef EPX_BROKER_HOOK_SUBSCRIBER_TRUNCATED
#define EPX_BROKER_HOOK_SUBSCRIBER_TRUNCATED(topic_str, dropped_count)  ((void)0)
#endif

#endif /* EPX_CONFIG_H */
