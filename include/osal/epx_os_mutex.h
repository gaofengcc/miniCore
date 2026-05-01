/**
 * @file epx_os_mutex.h
 * @brief OSAL recursive mutex. Same thread may lock multiple times.
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
 * @brief Create a recursive mutex.
 * @param mutex  Output mutex handle (must not be NULL).
 * @return EPX_OK on success, EPX_ERR_PARAM if mutex is NULL, EPX_ERR_NOMEM on failure.
 */
epx_err_t epx_os_mutex_create(epx_os_mutex_t* mutex);

/**
 * @brief Lock mutex (recursive: same thread may lock again).
 * @param mutex  Mutex handle (must not be NULL).
 * @return EPX_OK on success, EPX_ERR_PARAM if mutex is NULL.
 */
epx_err_t epx_os_mutex_lock(epx_os_mutex_t mutex);

/**
 * @brief Unlock mutex.
 * @param mutex  Mutex handle (must not be NULL).
 * @return EPX_OK on success, EPX_ERR_PARAM if mutex is NULL.
 */
epx_err_t epx_os_mutex_unlock(epx_os_mutex_t mutex);

/**
 * @brief Destroy mutex. No-op if mutex is NULL.
 * @param mutex  Mutex handle.
 */
void epx_os_mutex_destroy(epx_os_mutex_t* mutex);

#endif /* EPX_OSAL_ENABLE && EPX_OS_MUTEX_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_OS_MUTEX_H */
