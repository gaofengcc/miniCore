/**
 * @file epx_topic_tbl_priv.h
 * @brief Topic 注册表: 字符串到 id. 由 broker 与 topic_tbl 内部使用.
 */

#ifndef EPX_TOPIC_TBL_PRIV_H
#define EPX_TOPIC_TBL_PRIV_H

#include "epx_config.h"
#include "epx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(EPX_CORE_ENABLE)

/**
 * @brief 初始化 topic 表与互斥等内部状态.
 * @return 成功返回 EPX_OK.
 */
epx_err_t epx_topic_tbl_init(void);

/**
 * @brief 注册 topic; 已存在则返回已有 id.
 * @param topic_str  topic 字符串.
 * @param out_id     输出分配或查到的 id.
 * @return 成功返回 EPX_OK.
 */
epx_err_t epx_topic_tbl_register(const char* topic_str, uint16_t* out_id);

/**
 * @brief 按字符串查找已注册 topic 的 id.
 * @param topic_str  topic 字符串.
 * @param out_id     输出 id.
 * @return 找到返回 EPX_OK, 未找到返回 EPX_ERR.
 */
epx_err_t epx_topic_tbl_lookup(const char* topic_str, uint16_t* out_id);

#endif /* EPX_CORE_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_TOPIC_TBL_PRIV_H */
