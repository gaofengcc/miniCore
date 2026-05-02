/**
 * @file minimal_pub_sub.c
 * @brief miniCore Pub/Sub 使用示例.
 *
 * 此示例演示如何在 miniCore 中使用 Pub/Sub Broker:
 * - 初始化 Broker
 * - 创建消息队列并订阅 Topic
 * - 发布消息
 * - 接收并处理消息
 *
 * 构建要求:
 *   cmake -S . -B build
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* 轻量核心统一头文件 */
#include "minimal/epx_minimal.h"

/* 示例 Topic 定义 */
#define TOPIC_SENSOR_1_TEMP     "sensor/1/temp"
#define TOPIC_SENSOR_2_TEMP     "sensor/2/temp"
#define TOPIC_CONTROL_LED       "control/led"

/* 全局队列句柄 */
static epx_os_queue_t g_sensor_queue = NULL;
static epx_os_queue_t g_control_queue = NULL;

/**
 * @brief 尝试从队列中取出并打印所有当前消息
 */
static void drain_queue(epx_os_queue_t queue, const char* queue_name)
{
    while (1) {
        epx_msg_t msg = NULL;
        epx_err_t ret = epx_os_queue_recv(queue, &msg, 0);
        if (ret != EPX_OK) {
            break;
        }
        if (msg != NULL) {
            void* payload = epx_msg_payload(msg);
            size_t size = epx_msg_size(msg);
            printf("[%s] Received payload: size=%zu, data=%s\n",
                   queue_name, size, payload ? (char*)payload : "(null)");
            epx_msg_release(msg);
        }
    }
}

/**
 * @brief 模拟传感器数据发布
 */
static void simulate_sensor_publisher(void)
{
    static int counter = 0;
    char payload[64];

    /* 发布到 sensor/1/temp */
    snprintf(payload, sizeof(payload), "{\"id\":1,\"temp\":%.1f,\"seq\":%d}",
             25.0 + (counter % 10), counter);
    epx_publish_data(TOPIC_SENSOR_1_TEMP, payload, strlen(payload) + 1);
    printf("[Publisher] Published to %s: %s\n", TOPIC_SENSOR_1_TEMP, payload);

    /* 发布到 sensor/2/temp */
    snprintf(payload, sizeof(payload), "{\"id\":2,\"temp\":%.1f,\"seq\":%d}",
             28.0 + (counter % 5), counter);
    epx_publish_data(TOPIC_SENSOR_2_TEMP, payload, strlen(payload) + 1);
    printf("[Publisher] Published to %s: %s\n", TOPIC_SENSOR_2_TEMP, payload);

    counter++;
}

/**
 * @brief 模拟控制命令发布
 */
static void simulate_control_commands(void)
{
    const char* commands[] = {
        "{\"cmd\":\"on\",\"target\":\"led1\"}",
        "{\"cmd\":\"off\",\"target\":\"led1\"}",
        "{\"cmd\":\"toggle\",\"target\":\"led2\"}",
    };
    static int idx = 0;

    const char* cmd = commands[idx % 3];
    epx_publish_data(TOPIC_CONTROL_LED, cmd, strlen(cmd) + 1);
    printf("[Publisher] Published command to %s: %s\n", TOPIC_CONTROL_LED, cmd);

    idx++;
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    printf("=== miniCore Pub/Sub Example ===\n");
    printf("Version: %s\n", epx_minimal_version());
    printf("\n");

    /* 1. 初始化 Broker */
    printf("[Main] Initializing broker...\n");
    epx_err_t ret = epx_broker_init();
    if (ret != EPX_OK) {
        printf("[Main] Failed to initialize broker: %d\n", ret);
        return 1;
    }
    printf("[Main] Broker initialized successfully\n");

    /* 2. 创建消息队列 */
    ret = epx_os_queue_create(&g_sensor_queue, sizeof(epx_msg_t), 8);
    if (ret != EPX_OK) {
        printf("[Main] Failed to create sensor queue: %d\n", ret);
        return 1;
    }

    ret = epx_os_queue_create(&g_control_queue, sizeof(epx_msg_t), 4);
    if (ret != EPX_OK) {
        printf("[Main] Failed to create control queue: %d\n", ret);
        return 1;
    }

    /* 3. 订阅 Topic */
    printf("[Main] Subscribing to topics...\n");

    /* epx_subscribe_queue 基于队列订阅精确 topic, 这里将两个传感器 topic 绑定到同一接收队列 */
    ret = epx_subscribe_queue(TOPIC_SENSOR_1_TEMP, g_sensor_queue);
    if (ret != EPX_OK) {
        printf("[Main] Failed to subscribe to %s: %d\n", TOPIC_SENSOR_1_TEMP, ret);
        return 1;
    }
    printf("[Main] Subscribed to %s\n", TOPIC_SENSOR_1_TEMP);

    ret = epx_subscribe_queue(TOPIC_SENSOR_2_TEMP, g_sensor_queue);
    if (ret != EPX_OK) {
        printf("[Main] Failed to subscribe to %s: %d\n", TOPIC_SENSOR_2_TEMP, ret);
        return 1;
    }
    printf("[Main] Subscribed to %s\n", TOPIC_SENSOR_2_TEMP);

    /* 订阅 LED 控制命令 */
    ret = epx_subscribe_queue(TOPIC_CONTROL_LED, g_control_queue);
    if (ret != EPX_OK) {
        printf("[Main] Failed to subscribe to %s: %d\n", TOPIC_CONTROL_LED, ret);
        return 1;
    }
    printf("[Main] Subscribed to %s\n", TOPIC_CONTROL_LED);

    printf("\n[Main] Starting message loop...\n\n");

    /* 4. 主循环: 模拟发布消息并轮询队列 */
    int loop_count = 0;
    while (loop_count < 10) {
        simulate_sensor_publisher();
        drain_queue(g_sensor_queue, "SensorQueue");

        if (loop_count % 3 == 0) {
            simulate_control_commands();
            drain_queue(g_control_queue, "ControlQueue");
        }

        usleep(500000); /* 500ms */
        loop_count++;
    }

    printf("\n[Main] Cleaning up...\n");

    /* 5. 清理资源 */
    epx_unsubscribe_queue(TOPIC_SENSOR_1_TEMP, g_sensor_queue);
    epx_unsubscribe_queue(TOPIC_SENSOR_2_TEMP, g_sensor_queue);
    epx_unsubscribe_queue(TOPIC_CONTROL_LED, g_control_queue);

    epx_os_queue_destroy(&g_sensor_queue);
    epx_os_queue_destroy(&g_control_queue);

    epx_broker_deinit();

    printf("[Main] Example completed.\n");
    return 0;
}
