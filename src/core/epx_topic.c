/**
 * @file epx_topic.c
 * @brief N 叉 topic 树, 支持通配符订阅/匹配 (+, #).
 *        子节点用定长数组而非链表, 平均查找接近 O(1).
 */

#include "core/epx_topic.h"
#include "core/epx_mempool.h"
#include "epx_config.h"
#include "osal/epx_os_mem.h"
#include "osal/epx_os_mutex.h"

#if defined(EPX_CORE_ENABLE)

#include <stdlib.h>
#include <string.h>

#define EPX_TOPIC_WILDCARD_SINGLE  "+"
#define EPX_TOPIC_WILDCARD_MULTI   "#"

/* 每节点子槽数量, 取质数以利哈希分布. */
#define EPX_TOPIC_CHILD_SLOTS      8

/* 通配子节点的专用下标, 快速路径 */
#define EPX_TOPIC_IDX_SINGLE      0
#define EPX_TOPIC_IDX_MULTI       1
#define EPX_TOPIC_IDX_NORMAL      2

typedef struct sub_entry {
    epx_msg_callback_t callback;
    void* user_data;
    struct sub_entry* next;
} sub_entry_t;

typedef struct topic_node {
    char* segment;
    uint8_t is_wildcard;       /* 1 表示段为 + 或 # */
    uint8_t is_multi;          /* 1 表示段为 # */
    /* 子节点定长数组, 避免每子节点单独 malloc */
    struct topic_node* children[EPX_TOPIC_CHILD_SLOTS];
    uint8_t child_count;
    sub_entry_t* subscribers;
} topic_node_t;

struct epx_subscriber_list {
    epx_msg_callback_t* callbacks;
    void** user_datas;
    uint32_t count;
    uint32_t cap;
};

static topic_node_t* g_root = NULL;
static epx_os_mutex_t g_topic_mutex = NULL;
static epx_mempool_t g_node_pool = NULL;

/* 段字符串的简单哈希 */
static uint32_t segment_hash(const char* segment)
{
    uint32_t hash = 5381;
    char c;
    while ((c = *segment++) != '\0') {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

/* 计算段对应的子槽下标 */
static uint8_t get_child_slot(const char* segment)
{
    if (segment[0] == '+' && segment[1] == '\0') {
        return EPX_TOPIC_IDX_SINGLE;
    }
    if (segment[0] == '#' && segment[1] == '\0') {
        return EPX_TOPIC_IDX_MULTI;
    }
    /* 普通段用哈希取槽 */
    return EPX_TOPIC_IDX_NORMAL + (segment_hash(segment) % (EPX_TOPIC_CHILD_SLOTS - EPX_TOPIC_IDX_NORMAL));
}

static topic_node_t* node_create(const char* segment)
{
    size_t len = strlen(segment) + 1;
    if (len > (size_t)EPX_TOPIC_STR_MAX_LEN) {
        return NULL;
    }
    topic_node_t* n = NULL;
    if (g_node_pool != NULL) {
        n = (topic_node_t*)epx_mempool_alloc(g_node_pool);
    }
    if (n == NULL) {
        n = (topic_node_t*)epx_os_malloc(sizeof(topic_node_t));
    }
    if (n == NULL) {
        return NULL;
    }
    n->segment = (char*)epx_os_malloc(len);
    if (n->segment == NULL) {
        if (g_node_pool != NULL) {
            epx_mempool_free(g_node_pool, n);
        } else {
            epx_os_free(n);
        }
        return NULL;
    }
    memcpy(n->segment, segment, len);
    n->is_wildcard = (segment[0] == '+' || segment[0] == '#');
    n->is_multi = (segment[0] == '#');
    memset(n->children, 0, sizeof(n->children));
    n->child_count = 0;
    n->subscribers = NULL;
    return n;
}

static void node_destroy(topic_node_t* node)
{
    if (node == NULL) {
        return;
    }
    /* 销毁所有子节点 */
    for (int i = 0; i < EPX_TOPIC_CHILD_SLOTS; i++) {
        if (node->children[i] != NULL) {
            node_destroy(node->children[i]);
        }
    }
    sub_entry_t* s = node->subscribers;
    while (s != NULL) {
        sub_entry_t* n = s->next;
        epx_os_free(s);
        s = n;
    }
    if (node->segment != NULL) {
        epx_os_free(node->segment);
    }
    if (g_node_pool != NULL) {
        epx_mempool_free(g_node_pool, node);
    } else {
        epx_os_free(node);
    }
}

static int segment_eq(const char* a, const char* b)
{
    if (a == NULL || b == NULL) {
        return (a == b);
    }
    return (strcmp(a, b) == 0);
}

static topic_node_t* node_find_child(topic_node_t* parent, const char* segment)
{
    if (parent == NULL || segment == NULL) {
        return NULL;
    }
    uint8_t slot = get_child_slot(segment);
    topic_node_t* child = parent->children[slot];
    if (child != NULL && segment_eq(child->segment, segment)) {
        return child;
    }
    /* 哈希冲突时线性查找, 好哈希下很少发生 */
    for (int i = 0; i < EPX_TOPIC_CHILD_SLOTS; i++) {
        if (i != slot && parent->children[i] != NULL) {
            if (segment_eq(parent->children[i]->segment, segment)) {
                return parent->children[i];
            }
        }
    }
    return NULL;
}

static topic_node_t* node_ensure_child(topic_node_t* parent, const char* segment)
{
    topic_node_t* c = node_find_child(parent, segment);
    if (c != NULL) {
        return c;
    }
    c = node_create(segment);
    if (c == NULL) {
        return NULL;
    }
    uint8_t slot = get_child_slot(segment);
    int linked = 0;
    if (parent->children[slot] == NULL) {
        parent->children[slot] = c;
        linked = 1;
    } else {
        /* 冲突时找空槽 */
        for (int i = EPX_TOPIC_IDX_NORMAL; i < EPX_TOPIC_CHILD_SLOTS; i++) {
            if (parent->children[i] == NULL) {
                parent->children[i] = c;
                linked = 1;
                break;
            }
        }
    }
    if (!linked) {
        node_destroy(c);
        return NULL;
    }
    parent->child_count++;
    return c;
}

static epx_err_t subscribe_at_node(topic_node_t* node, epx_msg_callback_t callback, void* user_data)
{
    sub_entry_t* e = (sub_entry_t*)epx_os_malloc(sizeof(sub_entry_t));
    if (e == NULL) {
        return EPX_ERR_NOMEM;
    }
    e->callback = callback;
    e->user_data = user_data;
    e->next = node->subscribers;
    node->subscribers = e;
    return EPX_OK;
}

static epx_err_t collect_subscribers(topic_node_t* node, epx_subscriber_list_t* list)
{
    for (sub_entry_t* s = node->subscribers; s != NULL; s = s->next) {
        if (list->count >= list->cap) {
            uint32_t new_cap = (list->cap == 0) ? 8 : list->cap * 2;
            epx_msg_callback_t* nc = (epx_msg_callback_t*)epx_os_malloc(sizeof(epx_msg_callback_t) * new_cap);
            void** nd = (void**)epx_os_malloc(sizeof(void*) * new_cap);
            if (nc == NULL || nd == NULL) {
                if (nc) {
                    epx_os_free(nc);
                }
                if (nd) {
                    epx_os_free(nd);
                }
                return EPX_ERR_NOMEM;
            }
            if (list->callbacks != NULL) {
                memcpy(nc, list->callbacks, list->count * sizeof(epx_msg_callback_t));
                memcpy(nd, list->user_datas, list->count * sizeof(void*));
                epx_os_free(list->callbacks);
                epx_os_free(list->user_datas);
            }
            list->callbacks = nc;
            list->user_datas = nd;
            list->cap = new_cap;
        }
        list->callbacks[list->count] = s->callback;
        list->user_datas[list->count] = s->user_data;
        list->count++;
    }
    return EPX_OK;
}

static void subscriber_list_clear_buffers(epx_subscriber_list_t* list)
{
    if (list == NULL) {
        return;
    }
    if (list->callbacks != NULL) {
        epx_os_free(list->callbacks);
        list->callbacks = NULL;
    }
    if (list->user_datas != NULL) {
        epx_os_free(list->user_datas);
        list->user_datas = NULL;
    }
    list->cap = 0;
    list->count = 0;
}

static epx_err_t match_recursive(topic_node_t* node, const char* const* segments, uint32_t seg_count,
                                 uint32_t seg_idx, epx_subscriber_list_t* list)
{
    if (node == NULL) {
        return EPX_OK;
    }
    /* 快速路径: 先检查 # 通配 */
    for (int i = 0; i < EPX_TOPIC_CHILD_SLOTS; i++) {
        topic_node_t* child = node->children[i];
        if (child != NULL && child->is_multi) {
            epx_err_t er = collect_subscribers(child, list);
            if (er != EPX_OK) {
                return er;
            }
            break;
        }
    }
    if (seg_idx >= seg_count) {
        return collect_subscribers(node, list);
    }

    const char* seg = segments[seg_idx];

    /* 先检查精确匹配 */
    topic_node_t* exact = node_find_child(node, seg);
    if (exact != NULL) {
        epx_err_t er = match_recursive(exact, segments, seg_count, seg_idx + 1, list);
        if (er != EPX_OK) {
            return er;
        }
    }

    /* 再检查 + 通配 */
    for (int i = 0; i < EPX_TOPIC_CHILD_SLOTS; i++) {
        topic_node_t* child = node->children[i];
        if (child != NULL && child->is_wildcard && !child->is_multi) {
            epx_err_t er = match_recursive(child, segments, seg_count, seg_idx + 1, list);
            if (er != EPX_OK) {
                return er;
            }
            break;
        }
    }
    return EPX_OK;
}

static uint32_t split_segments(const char* topic, char* buf, size_t buf_size,
                                const char** out_segments, uint32_t max_segments)
{
    if (topic == NULL || buf == NULL || out_segments == NULL || max_segments == 0) {
        return 0;
    }
    size_t len = strlen(topic);
    if (len >= buf_size) {
        return 0;
    }
    memcpy(buf, topic, len + 1);
    uint32_t n = 0;
    char* p = buf;
    while (n < max_segments && *p != '\0') {
        out_segments[n++] = p;
        while (*p != '\0' && *p != '/') {
            p++;
        }
        if (*p == '/') {
            *p = '\0';
            p++;
        }
    }
    if (n > 0 && out_segments[n - 1][0] == '\0') {
        n--;
    }
    return n;
}

epx_err_t epx_topic_init(void)
{
    if (g_root != NULL) {
        return EPX_OK;
    }
    if (g_topic_mutex == NULL) {
        epx_err_t ret = epx_os_mutex_create(&g_topic_mutex);
        if (ret != EPX_OK) {
            return ret;
        }
    }
    /* 为 topic 节点创建内存池 */
    if (g_node_pool == NULL) {
        g_node_pool = epx_mempool_create(sizeof(topic_node_t), EPX_MAX_TOPICS * 2);
    }
    g_root = node_create("");
    if (g_root == NULL) {
        return EPX_ERR_NOMEM;
    }
    return EPX_OK;
}

/**
 * @brief 反初始化 topic 树, 释放所有节点与订阅者. 由 epx_broker_deinit 调用.
 *        调用后, 在再次 epx_topic_init 之前 epx_topic_subscribe/epx_topic_match 将返回错误.
 */
void epx_topic_deinit(void)
{
    if (g_topic_mutex != NULL) {
        epx_os_mutex_lock(g_topic_mutex);
    }
    if (g_root != NULL) {
        node_destroy(g_root);
        g_root = NULL;
    }
    if (g_topic_mutex != NULL) {
        epx_os_mutex_unlock(g_topic_mutex);
        epx_os_mutex_destroy(&g_topic_mutex);
        g_topic_mutex = NULL;
    }
    if (g_node_pool != NULL) {
        epx_mempool_destroy(g_node_pool);
        g_node_pool = NULL;
    }
}

epx_err_t epx_topic_subscribe(const char* topic_filter, epx_msg_callback_t callback, void* user_data)
{
    if (topic_filter == NULL || callback == NULL) {
        return EPX_ERR_PARAM;
    }
    if (g_root == NULL || g_topic_mutex == NULL) {
        return EPX_ERR;
    }
    size_t flen = strlen(topic_filter);
    if (flen >= (size_t)EPX_TOPIC_STR_MAX_LEN) {
        return EPX_ERR_PARAM;
    }
    char* buf = (char*)epx_os_malloc(flen + 1);
    if (buf == NULL) {
        return EPX_ERR_NOMEM;
    }
    memcpy(buf, topic_filter, flen + 1);
    const char* segs[EPX_TOPIC_MAX_DEPTH];
    uint32_t n = 0;
    char* p = buf;
    while (n < (uint32_t)EPX_TOPIC_MAX_DEPTH && *p != '\0') {
        segs[n++] = p;
        while (*p != '\0' && *p != '/') {
            p++;
        }
        if (*p == '/') {
            *p = '\0';
            p++;
        }
    }
    if (n == 0) {
        epx_os_free(buf);
        return EPX_ERR_PARAM;
    }
    /* 仅允许末尾 #; 中间 # (如 "a/#/b") 非法. */
    for (uint32_t i = 0; i + 1 < n; i++) {
        if (strcmp(segs[i], EPX_TOPIC_WILDCARD_MULTI) == 0) {
            epx_os_free(buf);
            return EPX_ERR_PARAM;
        }
    }

    epx_os_mutex_lock(g_topic_mutex);
    topic_node_t* cur = g_root;
    for (uint32_t i = 0; i < n && cur != NULL; i++) {
        const char* seg = segs[i];
        if (seg[0] == '\0') {
            continue;
        }
        cur = node_ensure_child(cur, seg);
    }
    epx_err_t ret = EPX_ERR_NOMEM;
    if (cur != NULL) {
        ret = subscribe_at_node(cur, callback, user_data);
    }
    epx_os_mutex_unlock(g_topic_mutex);
    epx_os_free(buf);
    return ret;
}

epx_err_t epx_subscriber_list_create(epx_subscriber_list_t** out_list)
{
    if (out_list == NULL) {
        return EPX_ERR_PARAM;
    }
    epx_subscriber_list_t* list = (epx_subscriber_list_t*)epx_os_malloc(sizeof(epx_subscriber_list_t));
    if (list == NULL) {
        return EPX_ERR_NOMEM;
    }
    list->callbacks = NULL;
    list->user_datas = NULL;
    list->count = 0;
    list->cap = 0;
    *out_list = list;
    return EPX_OK;
}

void epx_subscriber_list_destroy(epx_subscriber_list_t* list)
{
    if (list == NULL) {
        return;
    }
    if (list->callbacks != NULL) {
        epx_os_free(list->callbacks);
    }
    if (list->user_datas != NULL) {
        epx_os_free(list->user_datas);
    }
    epx_os_free(list);
}

epx_err_t epx_topic_match(const char* topic, epx_subscriber_list_t* out_list)
{
    if (topic == NULL || out_list == NULL) {
        return EPX_ERR_PARAM;
    }
    if (g_root == NULL) {
        return EPX_OK;
    }
    size_t tlen = strlen(topic);
    if (tlen >= (size_t)EPX_TOPIC_STR_MAX_LEN) {
        return EPX_ERR_PARAM;
    }
    char* buf = (char*)epx_os_malloc(tlen + 1);
    if (buf == NULL) {
        return EPX_ERR_NOMEM;
    }
    const char* segs[EPX_TOPIC_MAX_DEPTH];
    uint32_t n = split_segments(topic, buf, tlen + 1, segs, EPX_TOPIC_MAX_DEPTH);

    out_list->count = 0;
    epx_os_mutex_lock(g_topic_mutex);
    epx_err_t mret = match_recursive(g_root, segs, n, 0, out_list);
    epx_os_mutex_unlock(g_topic_mutex);
    epx_os_free(buf);
    if (mret != EPX_OK) {
        subscriber_list_clear_buffers(out_list);
        return mret;
    }
    return EPX_OK;
}

uint32_t epx_subscriber_list_count(epx_subscriber_list_t* list)
{
    if (list == NULL) {
        return 0;
    }
    return list->count;
}

epx_err_t epx_subscriber_list_get(epx_subscriber_list_t* list, uint32_t index,
                                  epx_msg_callback_t* out_cb, void** out_ud)
{
    if (list == NULL) {
        return EPX_ERR_PARAM;
    }
    if (index >= list->count) {
        return EPX_ERR_PARAM;
    }
    if (out_cb != NULL) {
        *out_cb = list->callbacks[index];
    }
    if (out_ud != NULL) {
        *out_ud = list->user_datas[index];
    }
    return EPX_OK;
}

#endif /* EPX_CORE_ENABLE */
