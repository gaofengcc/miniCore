/**
 * @file epx_topic_tbl_priv.h
 * @brief Topic registry: string to id. Internal use by broker and topic_tbl.
 */

#ifndef EPX_TOPIC_TBL_PRIV_H
#define EPX_TOPIC_TBL_PRIV_H

#include "epx_config.h"
#include "epx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(EPX_CORE_ENABLE)

epx_err_t epx_topic_tbl_init(void);
epx_err_t epx_topic_tbl_register(const char* topic_str, uint16_t* out_id);
epx_err_t epx_topic_tbl_lookup(const char* topic_str, uint16_t* out_id);

#endif /* EPX_CORE_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_TOPIC_TBL_PRIV_H */
