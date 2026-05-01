/**
 * @file epx_os_type.h
 * @brief OSAL type definitions (Step1: status, time, handle aliases).
 */

#ifndef EPX_OS_TYPE_H
#define EPX_OS_TYPE_H

#include "epx_config.h"
#include "epx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(EPX_OSAL_ENABLE)

/** Step1 alias: status code, same as epx_err_t. */
typedef epx_err_t epx_status_t;

#define EPX_STATUS_OK         EPX_OK
#define EPX_STATUS_ERROR      EPX_ERR
#define EPX_STATUS_TIMEOUT    EPX_ERR_TIMEOUT
#define EPX_STATUS_NO_MEM     EPX_ERR_NOMEM

/** epx_time_t is defined in epx_types.h (millisecond monotonic). */

#endif /* EPX_OSAL_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_OS_TYPE_H */
