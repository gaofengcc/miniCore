/**
 * @file epx_os_mutex.h
 * @brief OSAL 递归互斥锁. 同一线程可多次加锁.
 */

#ifndef EPX_OS_MUTEX_H
#define EPX_OS_MUTEX_H

#include "epx_config.h"
#include "epx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(EPX_OSAL_ENABLE) && defined(EPX_OS_MUTEX_ENABLE)

struct epx_os_mutex;
typedef struct epx_os_mutex* epx_os_mutex_t;

/**
 * @brief 创建递归互斥锁.
 * @param mutex  输出互斥句柄 (不得为 NULL).
 * @return 成功返回 EPX_OK, mutex 为 NULL 返回 EPX_ERR_PARAM, 失败返回 EPX_ERR_NOMEM.
 */
epx_err_t epx_os_mutex_create(epx_os_mutex_t* mutex);

/**
 * @brief 加锁 (递归: 同一线程可再次加锁).
 * @param mutex  互斥句柄 (不得为 NULL).
 * @return 成功返回 EPX_OK, mutex 为 NULL 返回 EPX_ERR_PARAM.
 */
epx_err_t epx_os_mutex_lock(epx_os_mutex_t mutex);

/**
 * @brief 解锁.
 * @param mutex  互斥句柄 (不得为 NULL).
 * @return 成功返回 EPX_OK, mutex 为 NULL 返回 EPX_ERR_PARAM.
 */
epx_err_t epx_os_mutex_unlock(epx_os_mutex_t mutex);

/**
 * @brief 销毁互斥锁. mutex 为 NULL 时为空操作.
 * @param mutex  互斥句柄.
 */
void epx_os_mutex_destroy(epx_os_mutex_t* mutex);

#endif /* EPX_OSAL_ENABLE && EPX_OS_MUTEX_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_OS_MUTEX_H */
