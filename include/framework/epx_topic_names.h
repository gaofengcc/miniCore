/**
 * @file epx_topic_names.h
 * @brief 核心 Topic 命名定义. 仅包含 Pub/Sub Broker 和 RPC 核心所需的最小集合.
 *
 * 注意: 此文件已精简, 仅保留核心功能所需的 Topic 定义.
 * 约束: EPX_TOPIC_STR_MAX_LEN, EPX_TOPIC_MAX_DEPTH 见 epx_config.h.
 */

#ifndef EPX_TOPIC_NAMES_H
#define EPX_TOPIC_NAMES_H

/* -------------------------------------------------------------------------- */
/* 根命名空间前缀 (首段路径或与 strncmp 用的前缀)                                */
/* -------------------------------------------------------------------------- */

/** 系统与平台服务 (RPC, log, OTA, 面向 HAL). 外网网关常限制访问. */
#define EPX_TOPIC_PREFIX_SYS        "sys/"
/** 进程内或仅调试用; 外部客户端默认勿向此处发布. */
#define EPX_TOPIC_PREFIX_INTERNAL   "internal/"
/** 网关上下行与链路状态. */
#define EPX_TOPIC_PREFIX_NET        "net/"
/** 示例/产品业务域 (演示应用). */
#define EPX_TOPIC_PREFIX_HOME       "home/"
/** 对接上游云; 常构成为 EPX_TOPIC_PREFIX_CLOUD + 内层 topic. */
#define EPX_TOPIC_PREFIX_CLOUD      "cloud/"

/* -------------------------------------------------------------------------- */
/* RPC: 请求 topic 后的回复段 (见 epx_rpc.c).                                   */
/* 这是 RPC 核心功能的关键定义, 必须保留在此文件中.                              */
/* -------------------------------------------------------------------------- */

#define EPX_TOPIC_RPC_REPLY_SUFFIX  "/reply/"

/* -------------------------------------------------------------------------- */
/* 最小演示 (envMonitor TempApp) - 轻量核心示例用                               */
/* -------------------------------------------------------------------------- */

#define EPX_TOPIC_DEMO_ENV_TEMP     "sensor/temp"

#endif /* EPX_TOPIC_NAMES_H */
