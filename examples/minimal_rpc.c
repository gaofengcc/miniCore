/**
 * @file minimal_rpc.c
 * @brief miniCore RPC 使用示例.
 *
 * 此示例演示如何在 miniCore 中使用 RPC 功能:
 * - 初始化 Broker
 * - 注册 RPC 服务端
 * - 客户端发起 RPC 调用
 * - 服务端处理并响应
 *
 * 构建要求:
 *   cmake -S . -B build -DMINICORE_ENABLE_RPC=ON
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

/* 轻量核心统一头文件 */
#include "minimal/epx_minimal.h"

/* RPC 方法 Topic 定义 */
#define RPC_MATH_ADD            "math/add"
#define RPC_MATH_SUB            "math/sub"

/**
 * @brief RPC 请求结构 (简化示例)
 */
typedef struct {
    int a;
    int b;
} math_req_t;

/**
 * @brief RPC 响应结构
 */
typedef struct {
    int result;
    int status;  /* 0 = OK, other = error */
} math_resp_t;

/**
 * @brief 从 RPC 请求消息中解析 math_req_t 指针 (跳过 epx_rpc_header_t).
 * @param req_msg  请求消息.
 * @return 有效时返回请求体指针, 否则 NULL.
 */
static math_req_t* rpc_get_math_req(epx_msg_t req_msg)
{
    void* req_payload = epx_msg_payload(req_msg);
    size_t req_size = epx_msg_size(req_msg);

    if (req_payload == NULL) {
        return NULL;
    }
    if (req_size < (sizeof(epx_rpc_header_t) + sizeof(math_req_t))) {
        return NULL;
    }

    return (math_req_t*)((uint8_t*)req_payload + sizeof(epx_rpc_header_t));
}

/**
 * @brief 数学加法服务处理器
 *
 * RPC 请求格式: 直接内存映射的 math_req_t
 * RPC 响应格式: 直接内存映射的 math_resp_t
 */
static void math_add_handler(void* user_data, epx_msg_t req_msg)
{
    (void)user_data;

    printf("[RPC Server] math/add called, request size=%zu\n", epx_msg_size(req_msg));

    math_resp_t resp = {0, 0};
    math_req_t* req = rpc_get_math_req(req_msg);
    if (req != NULL) {
        resp.result = req->a + req->b;
        printf("[RPC Server] Computing: %d + %d = %d\n", req->a, req->b, resp.result);
    } else {
        printf("[RPC Server] Invalid request format\n");
        resp.status = -1;
    }

    /* 发送响应 */
    epx_err_t ret = epx_rpc_reply(req_msg, &resp, sizeof(resp));
    if (ret != EPX_OK) {
        printf("[RPC Server] Failed to send reply: %d\n", ret);
    } else {
        printf("[RPC Server] Reply sent successfully\n");
    }
}

/**
 * @brief 数学减法服务处理器
 */
static void math_sub_handler(void* user_data, epx_msg_t req_msg)
{
    (void)user_data;

    printf("[RPC Server] math/sub called, request size=%zu\n", epx_msg_size(req_msg));

    math_resp_t resp = {0, 0};
    math_req_t* req = rpc_get_math_req(req_msg);
    if (req != NULL) {
        resp.result = req->a - req->b;
        printf("[RPC Server] Computing: %d - %d = %d\n", req->a, req->b, resp.result);
    } else {
        printf("[RPC Server] Invalid request format\n");
        resp.status = -1;
    }

    epx_err_t ret = epx_rpc_reply(req_msg, &resp, sizeof(resp));
    if (ret != EPX_OK) {
        printf("[RPC Server] Failed to send reply: %d\n", ret);
    } else {
        printf("[RPC Server] Reply sent successfully\n");
    }
}

/**
 * @brief 启动 RPC 服务端
 */
static int start_rpc_server(void)
{
    printf("[Main] Starting RPC server...\n");

    /* 注册加法服务 */
    epx_err_t ret = epx_rpc_register(RPC_MATH_ADD, math_add_handler, NULL);
    if (ret != EPX_OK) {
        printf("[Main] Failed to register math/add: %d\n", ret);
        return -1;
    }
    printf("[Main] Registered RPC service: %s\n", RPC_MATH_ADD);

    /* 注册减法服务 */
    ret = epx_rpc_register(RPC_MATH_SUB, math_sub_handler, NULL);
    if (ret != EPX_OK) {
        printf("[Main] Failed to register math/sub: %d\n", ret);
        return -1;
    }
    printf("[Main] Registered RPC service: %s\n", RPC_MATH_SUB);

    return 0;
}

/**
 * @brief RPC 客户端调用示例
 */
static void rpc_client_call_example(void)
{
    math_req_t req;
    math_resp_t resp;
    epx_err_t ret;

    /* 示例 1: 调用加法服务 */
    printf("\n[RPC Client] Calling math/add(3, 5)...\n");

    req.a = 3;
    req.b = 5;
    memset(&resp, 0, sizeof(resp));

    ret = epx_rpc_call(RPC_MATH_ADD, &req, sizeof(req), &resp, sizeof(resp), 2000);

    if (ret == EPX_OK) {
        printf("[RPC Client] Result: %d (status=%d)\n", resp.result, resp.status);
    } else if (ret == EPX_ERR_TIMEOUT) {
        printf("[RPC Client] Call timeout\n");
    } else {
        printf("[RPC Client] Call failed: %d\n", ret);
    }

    /* 示例 2: 调用减法服务 */
    printf("\n[RPC Client] Calling math/sub(10, 4)...\n");

    req.a = 10;
    req.b = 4;
    memset(&resp, 0, sizeof(resp));

    ret = epx_rpc_call(RPC_MATH_SUB, &req, sizeof(req), &resp, sizeof(resp), 2000);

    if (ret == EPX_OK) {
        printf("[RPC Client] Result: %d (status=%d)\n", resp.result, resp.status);
    } else if (ret == EPX_ERR_TIMEOUT) {
        printf("[RPC Client] Call timeout\n");
    } else {
        printf("[RPC Client] Call failed: %d\n", ret);
    }

    /* 示例 3: 测试超时场景 (调用不存在的服务) */
    printf("\n[RPC Client] Calling non-existent service (should timeout)...\n");

    ret = epx_rpc_call("math/mul", &req, sizeof(req), &resp, sizeof(resp), 500);

    if (ret == EPX_ERR_TIMEOUT) {
        printf("[RPC Client] Expected timeout occurred\n");
    } else {
        printf("[RPC Client] Unexpected result: %d\n", ret);
    }
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    printf("=== miniCore RPC Example ===\n");
    printf("Version: %s\n", epx_minimal_version());
    printf("\n");

#if !defined(EPX_RPC_ENABLE)
    printf("ERROR: This example requires RPC enabled.\n");
    printf("Please build with: -DMINICORE_ENABLE_RPC=ON\n");
    return 1;
#endif

    /* 1. 初始化 Broker */
    printf("[Main] Initializing broker...\n");
    epx_err_t ret = epx_broker_init();
    if (ret != EPX_OK) {
        printf("[Main] Failed to initialize broker: %d\n", ret);
        return 1;
    }
    printf("[Main] Broker initialized successfully\n");

    /* 2. 启动 RPC 服务端 */
    if (start_rpc_server() != 0) {
        printf("[Main] Failed to start RPC server\n");
        epx_broker_deinit();
        return 1;
    }

    printf("\n[Main] RPC server ready\n");

    /* 3. 等待服务就绪 (给服务端线程时间启动) */
    usleep(100000);  /* 100ms */

    /* 4. 执行 RPC 客户端调用 */
    rpc_client_call_example();

    printf("\n[Main] Example completed.\n");

    /* 注意: 在真实应用中, 这里应该清理 RPC 服务端资源.
     * 当前 EpxCore RPC 实现中, 服务端线程是无限循环的,
     * 清理需要额外的 RPC 去注册功能 (未在此示例中实现).
     */

    epx_broker_deinit();
    return 0;
}
