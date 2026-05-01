/**
 * @file epx_topic_names.h
 * @brief 核心 Topic 命名定义. 仅包含 Pub/Sub Broker 和 RPC 核心所需的最小集合.
 *
 * 注意: 此文件已精简, 仅保留核心功能所需的 Topic 定义.
 * Constraints: EPX_TOPIC_STR_MAX_LEN, EPX_TOPIC_MAX_DEPTH in epx_config.h.
 */

#ifndef EPX_TOPIC_NAMES_H
#define EPX_TOPIC_NAMES_H

/* -------------------------------------------------------------------------- */
/* Root namespace prefixes (first path segment or prefix for strncmp)         */
/* -------------------------------------------------------------------------- */

/** System and platform services (RPC, log, OTA, HAL-facing). Often restricted on external gateways. */
#define EPX_TOPIC_PREFIX_SYS        "sys/"
/** Process-internal or debug-only; external clients must not publish here by default. */
#define EPX_TOPIC_PREFIX_INTERNAL   "internal/"
/** Gateway uplink/downlink and link status. */
#define EPX_TOPIC_PREFIX_NET        "net/"
/** Example / product business domain (demo apps). */
#define EPX_TOPIC_PREFIX_HOME       "home/"
/** Bridge to upstream cloud; often built as EPX_TOPIC_PREFIX_CLOUD + inner topic. */
#define EPX_TOPIC_PREFIX_CLOUD      "cloud/"

/* -------------------------------------------------------------------------- */
/* RPC: reply topic segment after request topic (see epx_rpc.c).              */
/* 这是 RPC 核心功能的关键定义, 必须保留在此文件中.                              */
/* -------------------------------------------------------------------------- */

#define EPX_TOPIC_RPC_REPLY_SUFFIX  "/reply/"

/* -------------------------------------------------------------------------- */
/* Minimal demo (envMonitor TempApp) - 轻量核心示例用                            */
/* -------------------------------------------------------------------------- */

#define EPX_TOPIC_DEMO_ENV_TEMP     "sensor/temp"

#endif /* EPX_TOPIC_NAMES_H */
