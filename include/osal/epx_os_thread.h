/**
 * @file epx_os_thread.h
 * @brief OSAL 线程/任务抽象. 语义接近 POSIX.
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
 * @brief 全局 OSAL 初始化. 使用其它 OSAL API 前仅调用一次.
 * @return 成功返回 EPX_OK.
 */
epx_err_t epx_os_init(void);

/**
 * @brief 取得单调时钟毫秒数.
 * @return 自任意纪元起的毫秒数 (单调, 非挂钟).
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
 * @brief 创建线程.
 * @param handle     输出线程句柄 (不得为 NULL).
 * @param name       调试用可选名称 (可为 NULL).
 * @param stack_size 栈大小, 字节 (Linux 上忽略, 为 MCU 预留).
 * @param prio       优先级 (映射到底层 OS).
 * @param entry      线程入口函数.
 * @param arg        传入 entry 的参数.
 * @return 成功返回 EPX_OK, handle/entry 为 NULL 返回 EPX_ERR_PARAM, 失败返回 EPX_ERR_NOMEM.
 */
epx_err_t epx_os_thread_create(epx_os_thread_handle_t* handle,
                               const char* name,
                               uint32_t stack_size,
                               epx_os_prio_t prio,
                               epx_os_thread_entry_t entry,
                               void* arg);

/**
 * @brief 等待线程退出.
 * @param handle  线程句柄 (不得为 NULL).
 * @param result  可选输出线程返回值 (可为 NULL).
 * @return 成功返回 EPX_OK, handle 为 NULL 返回 EPX_ERR_PARAM.
 */
epx_err_t epx_os_thread_join(epx_os_thread_handle_t handle, void** result);

/**
 * @brief 当前线程延时指定毫秒.
 * @param ms  延时毫秒数.
 */
void epx_os_delay(uint32_t ms);

/**
 * @brief 销毁线程句柄 (在 join 之后). handle 为 NULL 时为空操作.
 * @param handle  线程句柄.
 */
void epx_os_thread_destroy(epx_os_thread_handle_t* handle);

/**
 * @brief 打印, 带时间戳前缀 [sec.ms] [INFO] user_fmt...
 * @param fmt  格式串 (printf 风格).
 */
void epx_os_printf(const char* fmt, ...);

#endif /* EPX_OSAL_ENABLE && EPX_OS_THREAD_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_OS_THREAD_H */
