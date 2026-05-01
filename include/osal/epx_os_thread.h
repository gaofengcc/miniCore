/**
 * @file epx_os_thread.h
 * @brief OSAL thread/task abstraction. POSIX-like semantics.
 */

#ifndef EPX_OS_THREAD_H
#define EPX_OS_THREAD_H

#include "epx_config.h"
#include "epx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(EPX_OSAL_ENABLE) && defined(EPX_OS_THREAD_ENABLE)

/**
 * @brief Global OSAL init. Call once before using other OSAL APIs.
 * @return EPX_OK on success.
 */
epx_err_t epx_os_init(void);

/**
 * @brief Get monotonic time in milliseconds.
 * @return Milliseconds since an arbitrary epoch (monotonic, not wall clock).
 */
epx_time_t epx_os_get_time_ms(void);

typedef void* (*epx_os_thread_entry_t)(void* arg);

typedef enum {
    EPX_OS_PRIO_LOWEST = 0,
    EPX_OS_PRIO_LOW,
    EPX_OS_PRIO_NORMAL,
    EPX_OS_PRIO_HIGH,
    EPX_OS_PRIO_HIGHEST,
    EPX_OS_PRIO_COUNT
} epx_os_prio_t;

struct epx_os_thread;
typedef struct epx_os_thread* epx_os_thread_handle_t;

/**
 * @brief Create a thread.
 * @param handle     Output thread handle (must not be NULL).
 * @param name       Optional name for debugging (can be NULL).
 * @param stack_size Stack size in bytes (ignored on Linux, reserved for MCU).
 * @param prio       Priority (mapped to underlying OS).
 * @param entry      Thread entry function.
 * @param arg        Argument passed to entry.
 * @return EPX_OK on success, EPX_ERR_PARAM if handle/entry is NULL, EPX_ERR_NOMEM on failure.
 */
epx_err_t epx_os_thread_create(epx_os_thread_handle_t* handle,
                               const char* name,
                               uint32_t stack_size,
                               epx_os_prio_t prio,
                               epx_os_thread_entry_t entry,
                               void* arg);

/**
 * @brief Wait for thread to exit.
 * @param handle  Thread handle (must not be NULL).
 * @param result  Optional output for thread return value (can be NULL).
 * @return EPX_OK on success, EPX_ERR_PARAM if handle is NULL.
 */
epx_err_t epx_os_thread_join(epx_os_thread_handle_t handle, void** result);

/**
 * @brief Delay current thread for given milliseconds.
 * @param ms  Delay in milliseconds.
 */
void epx_os_delay(uint32_t ms);

/**
 * @brief Destroy thread handle (after join). No-op if handle is NULL.
 * @param handle  Thread handle.
 */
void epx_os_thread_destroy(epx_os_thread_handle_t* handle);

/**
 * @brief Print with timestamp prefix [sec.ms] [INFO] user_fmt...
 * @param fmt  Format string (printf-style).
 */
void epx_os_printf(const char* fmt, ...);

#endif /* EPX_OSAL_ENABLE && EPX_OS_THREAD_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_OS_THREAD_H */
