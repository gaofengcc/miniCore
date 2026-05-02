/**
 * @file broker_smoke.c
 * @brief 最小冒烟: broker 初始化, 订阅, 发布, 接收, 析构.
 */

#include <assert.h>
#include <string.h>

#include "minimal/epx_minimal.h"

int main(void)
{
    epx_err_t ret;
    epx_os_queue_t q = NULL;
    const char* topic = "test/smoke/ping";
    const char payload[] = "ok";

    ret = epx_broker_init();
    assert(ret == EPX_OK);

    ret = epx_os_queue_create(&q, sizeof(epx_msg_t), 4);
    assert(ret == EPX_OK && q != NULL);

    ret = epx_sub(topic, q);
    assert(ret == EPX_OK);

    ret = epx_pub(topic, payload, sizeof(payload));
    assert(ret == EPX_OK);

    epx_msg_t msg = NULL;
    ret = epx_os_queue_recv(q, &msg, 2000);
    assert(ret == EPX_OK && msg != NULL);
    assert(epx_msg_size(msg) == sizeof(payload));
    assert(memcmp(epx_msg_payload(msg), payload, sizeof(payload)) == 0);
    epx_msg_release(msg);

    ret = epx_unsub(topic, q);
    assert(ret == EPX_OK);

    epx_os_queue_destroy(&q);
    epx_broker_deinit();
    return 0;
}
