/**
 * @file epx_minimal.h
 * @brief miniCore 统一头文件. 仅包含 Pub/Sub Broker 和可选 RPC 的核心功能.
 *
 * 这是独立 miniCore 仓库的入口头文件, 已剥离 Services、HAL、Gateway 等非核心模块.
 * 适用于需要轻量化 Pub/Sub 内核集成的嵌入式项目.
 *
 * 使用此头文件的前提:
 * - 使用 miniCore 仓库直接构建
 * - 仅需进程内 Pub/Sub 消息总线功能
 * - 可选 RPC 远程调用功能
 *
 * 典型使用流程:
 * @code
 *   #include "minimal/epx_minimal.h"
 *
 *   // 1. 初始化 Broker
 *   epx_broker_init();
 *
 *   // 2. 创建队列并订阅 Topic
 *   epx_os_queue_t my_queue;
 *   epx_os_queue_create(&my_queue, sizeof(epx_msg_t), 8);
 *   epx_subscribe_queue("sensor/1/temp", my_queue);
 *
 *   // 3. 发布消息 (原始负载; 通配订阅请用 epx_subscribe + epx_publish)
 *   epx_publish_data("sensor/1/temp", data, len);
 *
 *   // 4. 在接收线程中处理消息
 *   epx_msg_t msg;
 *   if (epx_os_queue_recv(my_queue, &msg, 100) == EPX_OK) {
 *       // 处理消息...
 *       epx_msg_release(msg);
 *   }
 * @endcode
 *
 * @see epx_broker.h
 * @see epx_rpc.h (如果启用 RPC)
 */

#ifndef EPX_MINIMAL_H
#define EPX_MINIMAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 基础类型与配置
 * ============================================================================ */

#include "epx_types.h"
#include "epx_config.h"
#include "minicore_version.h"

/* ============================================================================
 * OSAL 层 (操作系统抽象)
 * ============================================================================ */

#include "osal/epx_os_mem.h"
#include "osal/epx_os_mutex.h"
#include "osal/epx_os_queue.h"
#include "osal/epx_os_thread.h"

/* ============================================================================
 * Core 层 (Pub/Sub Broker 内核)
 * ============================================================================ */

#include "core/epx_msg.h"
#include "core/epx_topic.h"
#include "core/epx_broker.h"

/* ============================================================================
 * Framework 层 (可选 RPC)
 * ============================================================================ */

#if defined(EPX_FRAMEWORK_ENABLE)
    #include "framework/epx_topic_names.h"
#endif

#if defined(EPX_RPC_ENABLE)
    #include "framework/epx_rpc.h"
#endif

/* ============================================================================
 * 版本信息 (与 include/minicore_version.h 及 CMake project(VERSION) 对齐)
 * ============================================================================ */

#define EPX_MINIMAL_VERSION_MAJOR   MINICORE_VERSION_MAJOR
#define EPX_MINIMAL_VERSION_MINOR   MINICORE_VERSION_MINOR
#define EPX_MINIMAL_VERSION_PATCH   MINICORE_VERSION_PATCH

/**
 * @brief 获取轻量核心版本字符串
 * @return 版本字符串, 如 "0.1.0"
 */
static inline const char* epx_minimal_version(void)
{
    return MINICORE_VERSION_STRING;
}

/**
 * @brief 检查轻量核心功能是否可用
 * @return 1 表示轻量核心已正确初始化, 0 表示未初始化
 */
static inline int epx_minimal_is_ready(void)
{
    return epx_broker_is_ready();
}

#ifdef __cplusplus
}
#endif

#endif /* EPX_MINIMAL_H */
