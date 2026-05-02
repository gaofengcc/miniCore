/**
 * @file broker_wildcard_smoke.c
 * @brief 通配符订阅 (topic 树) + epx_publish 投递到回调.
 */

#include <assert.h>
#include <string.h>

#include "minimal/epx_minimal.h"

static volatile int g_cb_count;

static void on_msg(epx_msg_t msg, void* user_data)
{
    (void)user_data;
    if (msg != NULL && epx_msg_topic(msg) != NULL &&
        strcmp(epx_msg_topic(msg), "wild/a/x") == 0) {
        g_cb_count++;
    }
}

int main(void)
{
    g_cb_count = 0;
    assert(epx_broker_init() == EPX_OK);
    assert(epx_subscribe("wild/#", on_msg, NULL) == EPX_OK);

    static const char topic[] = "wild/a/x";
    epx_msg_t m = epx_msg_new(topic, 4);
    assert(m != NULL);
    memcpy(epx_msg_payload(m), "data", 4);
    assert(epx_publish(m) == EPX_OK);
    assert(g_cb_count == 1);

    epx_broker_deinit();
    return 0;
}
