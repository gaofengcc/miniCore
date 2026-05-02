/**
 * @file epx_msg.h
 * @brief 零拷贝消息块: ref_count, 负载, 可选析构函数.
 *        支持内存池以降低分配开销.
 */

#ifndef EPX_MSG_H
#define EPX_MSG_H

#include "epx_config.h"
#include "epx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(EPX_CORE_ENABLE)

struct epx_msg_block;
typedef struct epx_msg_block* epx_msg_t;

/**
 * @brief 初始化消息模块 (ref_count 全局互斥). 在 alloc/retain/release 前仅调用一次.
 * @return 成功返回 EPX_OK.
 */
epx_err_t epx_msg_init(void);

/**
 * @brief 使用小消息内存池初始化消息模块.
 *        单池块须容纳 sizeof(epx_msg_block) + 负载; 创建后见 epx_mempool_block_size.
 *        若需在自定义池下使用 epx_msg_alloc / epx_msg_new, 须在其之前调用;
 *        若已执行 epx_msg_init, 本函数直接返回且不会应用自定义池 (与历史行为一致).
 * @param pool_block_size   请求的池块大小 (如 128, 256), 内部会对齐.
 * @param pool_block_count  池中块数量.
 * @return 成功返回 EPX_OK.
 */
epx_err_t epx_msg_init_with_pool(size_t pool_block_size, uint32_t pool_block_count);

/**
 * @brief 从池 (若有) 或堆分配消息块.
 *        ref_count = 1. 负载大小为 size 字节. 无 topic.
 * @param size  负载大小, 字节.
 * @return 消息句柄, 失败返回 NULL.
 */
epx_msg_t epx_msg_alloc(size_t size);

/**
 * @brief 创建带 topic 与负载的消息. 零拷贝: 直接保存 topic 指针.
 *        ref_count = 1. topic 字符串不拷贝 (仅存指针), 预留 payload_len 字节负载区.
 *        topic 指针须在本句柄最后一次 epx_msg_release 之前保持有效 (含 Broker 或订阅方的 retain/release).
 *        除非使用 epx_msg_new_copy 或保证数据存活, 勿传入栈上或短生命周期缓冲区.
 * @param topic         topic 字符串 (仅存指针, 不拷贝, 最大 EPX_TOPIC_STR_MAX_LEN).
 *                      不得为 NULL.
 * @param payload_len   负载大小, 字节.
 * @return 消息句柄, 失败返回 NULL.
 */
epx_msg_t epx_msg_new(const char* topic, size_t payload_len);

/**
 * @brief 创建消息并拷贝 topic (兼容旧行为的零拷贝替代).
 * @param topic         topic 字符串 (拷贝, 最大 EPX_TOPIC_STR_MAX_LEN). 不得为 NULL.
 * @param payload_len   负载大小, 字节.
 * @return 消息句柄, 失败返回 NULL.
 */
epx_msg_t epx_msg_new_copy(const char* topic, size_t payload_len);

/**
 * @brief 增加 ref_count. msg 为 NULL 时安全 (空操作).
 * @param msg  消息句柄.
 */
void epx_msg_retain(epx_msg_t msg);

/**
 * @brief 减少 ref_count. 为 0 时调用析构 (若已设置) 并释放. NULL 安全.
 * @param msg  消息句柄. release 后不得再使用.
 */
void epx_msg_release(epx_msg_t msg);

/**
 * @brief 设置 ref_count 到 0 时调用的析构函数 (在 free 之前). 可选, 用于测试或自定义清理.
 * @param msg         消息句柄 (不得为 NULL).
 * @param destructor  以块指针调用的函数, 或 NULL 表示清除.
 */
void epx_msg_set_destructor(epx_msg_t msg, void (*destructor)(void*));

/**
 * @brief 取得负载指针 (只读). epx_msg_release 后勿再使用.
 * @param msg  消息句柄 (不得为 NULL).
 * @return 指向负载 (data[]) 的指针.
 */
void* epx_msg_payload(epx_msg_t msg);

/**
 * @brief 取得负载大小, 字节.
 * @param msg  消息句柄 (不得为 NULL).
 * @return 负载大小.
 */
size_t epx_msg_size(epx_msg_t msg);

/**
 * @brief 取得 topic 字符串. 若消息由 epx_msg_alloc 创建 (无 topic) 则返回 NULL.
 * @param msg  消息句柄 (不得为 NULL).
 * @return topic 字符串, 或 NULL.
 */
const char* epx_msg_topic(epx_msg_t msg);

#endif /* EPX_CORE_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* EPX_MSG_H */
