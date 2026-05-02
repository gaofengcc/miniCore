/**
 * @file epx_os_type.h
 * @brief OSAL 类型定义 (Step1: 状态码, 时间, 句柄别名).
 */

#ifndef EPX_OS_TYPE_H
#define EPX_OS_TYPE_H

#include "epx_config.h"
#include "epx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(EPX_OSAL_ENABLE)

/** Step1 别名: 状态码, 与 epx_err_t 相同. */
typedef epx_err_t epx_status_t;

#define EPX_STATUS_OK         EPX_OK
#define EPX_STATUS_ERROR      EPX_ERR
#define EPX_STATUS_TIMEOUT    EPX_ERR_TIMEOUT
#define EPX_STATUS_NO_MEM     EPX_ERR_NOMEM

/** epx_time_t 在 epx_types.h 中定义 (单调毫秒). */

#endif /* EPX_OSAL_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_OS_TYPE_H */
