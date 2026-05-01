/**
 * @file epx_topic_tbl.c
 * @brief Topic registry: DJB2 hash + dynamic array, string compare for exact match.
 *        Thread-safe: register/lookup protected by mutex so concurrent epx_sub same topic get same id.
 */

#include "core/epx_topic_tbl_priv.h"
#include "epx_config.h"
#include "osal/epx_os_mem.h"
#include "osal/epx_os_mutex.h"

#if defined(EPX_CORE_ENABLE)

#include <stdlib.h>
#include <string.h>

typedef struct {
    char* str;
    uint16_t id;
} topic_entry_t;

static topic_entry_t* g_entries = NULL;
static uint32_t g_count = 0;
static uint32_t g_cap = 0;
static uint16_t g_next_id = 0;
static epx_os_mutex_t g_tbl_mutex = NULL;

/* DJB2 hash for quick bucket/compare hint. Full match still by strcmp. */
static unsigned long djb2_hash(const char* str)
{
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++) != '\0') {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

epx_err_t epx_topic_tbl_init(void)
{
    if (g_entries != NULL) {
        return EPX_OK;
    }
    if (g_tbl_mutex == NULL) {
        epx_err_t ret = epx_os_mutex_create(&g_tbl_mutex);
        if (ret != EPX_OK || g_tbl_mutex == NULL) {
            return (ret != EPX_OK) ? ret : EPX_ERR;
        }
    }
    epx_os_mutex_lock(g_tbl_mutex);
    if (g_entries != NULL) {
        epx_os_mutex_unlock(g_tbl_mutex);
        return EPX_OK;
    }
    g_cap = (uint32_t)EPX_MAX_TOPICS;
    g_entries = (topic_entry_t*)epx_os_malloc(sizeof(topic_entry_t) * g_cap);
    if (g_entries == NULL) {
        epx_os_mutex_unlock(g_tbl_mutex);
        return EPX_ERR_NOMEM;
    }
    memset(g_entries, 0, sizeof(topic_entry_t) * g_cap);
    g_count = 0;
    g_next_id = 0;
    epx_os_mutex_unlock(g_tbl_mutex);
    return EPX_OK;
}

epx_err_t epx_topic_tbl_register(const char* topic_str, uint16_t* out_id)
{
    if (topic_str == NULL || out_id == NULL) {
        return EPX_ERR_PARAM;
    }
    size_t len = strlen(topic_str);
    if (len >= (size_t)EPX_TOPIC_STR_MAX_LEN) {
        return EPX_ERR_PARAM;
    }
    if (g_entries == NULL || g_tbl_mutex == NULL) {
        return EPX_ERR;
    }
    epx_os_mutex_lock(g_tbl_mutex);
    unsigned long hash = djb2_hash(topic_str);
    len++;
    for (uint32_t i = 0; i < g_count; i++) {
        if (g_entries[i].str != NULL && djb2_hash(g_entries[i].str) == hash && strcmp(g_entries[i].str, topic_str) == 0) {
            *out_id = g_entries[i].id;
            epx_os_mutex_unlock(g_tbl_mutex);
            return EPX_OK;
        }
    }
    if (g_count >= g_cap) {
        epx_os_mutex_unlock(g_tbl_mutex);
        return EPX_ERR_NOMEM;
    }
    char* copy = (char*)epx_os_malloc(len);
    if (copy == NULL) {
        epx_os_mutex_unlock(g_tbl_mutex);
        return EPX_ERR_NOMEM;
    }
    memcpy(copy, topic_str, len);
    g_entries[g_count].str = copy;
    g_entries[g_count].id = g_next_id++;
    *out_id = g_entries[g_count].id;
    g_count++;
    epx_os_mutex_unlock(g_tbl_mutex);
    return EPX_OK;
}

epx_err_t epx_topic_tbl_lookup(const char* topic_str, uint16_t* out_id)
{
    if (topic_str == NULL || out_id == NULL) {
        return EPX_ERR_PARAM;
    }
    if (strlen(topic_str) >= (size_t)EPX_TOPIC_STR_MAX_LEN) {
        return EPX_ERR_PARAM;
    }
    if (g_entries == NULL || g_tbl_mutex == NULL) {
        return EPX_ERR;
    }
    epx_os_mutex_lock(g_tbl_mutex);
    unsigned long hash = djb2_hash(topic_str);
    for (uint32_t i = 0; i < g_count; i++) {
        if (g_entries[i].str != NULL && djb2_hash(g_entries[i].str) == hash && strcmp(g_entries[i].str, topic_str) == 0) {
            *out_id = g_entries[i].id;
            epx_os_mutex_unlock(g_tbl_mutex);
            return EPX_OK;
        }
    }
    epx_os_mutex_unlock(g_tbl_mutex);
    return EPX_ERR;
}

#endif /* EPX_CORE_ENABLE */
