/**
 * @file broker_trunc_smoke.c
 * @brief More than EPX_MAX_SUBSCRIBERS_PER_TOPIC queue subscribers: epx_pub returns EPX_ERR_BUSY.
 */

#include <assert.h>
#include <stdint.h>

#include "epx_config.h"
#include "minimal/epx_minimal.h"

#define EXTRA_SUBS  ((EPX_MAX_SUBSCRIBERS_PER_TOPIC) + 1)

int main(void)
{
    assert(epx_broker_init() == EPX_OK);

    const char topic[] = "trunc/topic/exceed";
    epx_os_queue_t qs[EXTRA_SUBS];

    for (uint32_t i = 0; i < EXTRA_SUBS; i++) {
        qs[i] = NULL;
        assert(epx_os_queue_create(&qs[i], sizeof(epx_msg_t), 2) == EPX_OK && qs[i] != NULL);
        assert(epx_sub(topic, qs[i]) == EPX_OK);
    }

    const char payload[] = "x";
    assert(epx_pub(topic, payload, sizeof(payload)) == EPX_ERR_BUSY);

    for (uint32_t i = 0; i < EXTRA_SUBS; i++) {
        assert(epx_unsub(topic, qs[i]) == EPX_OK);
        epx_os_queue_destroy(&qs[i]);
    }

    epx_broker_deinit();
    return 0;
}
