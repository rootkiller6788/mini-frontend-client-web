#include "react_native_bridge.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

rn_status_t rn_bridge_init(rn_bridge_t *bridge)
{
    if (!bridge) return RN_ERR_BAD_ARGS;
    memset(bridge, 0, sizeof(*bridge));

    bridge->queue = (struct rn_bridge_msg *)calloc(RN_BRIDGE_QUEUE_SIZE,
                                                   sizeof(struct rn_bridge_msg));
    if (!bridge->queue) return RN_ERR_BAD_ARGS;

    bridge->queue_head    = 0;
    bridge->queue_tail    = 0;
    bridge->queue_count   = 0;
    bridge->msg_seq       = 1;
    bridge->module_count  = 0;
    bridge->callback_count= 0;
    bridge->callback_seq  = 1;
    bridge->batching      = true;

    return RN_OK;
}

void rn_bridge_destroy(rn_bridge_t *bridge)
{
    if (!bridge) return;
    free(bridge->queue);
    bridge->queue = NULL;
    memset(bridge, 0, sizeof(*bridge));
}

rn_status_t rn_bridge_register_module(rn_bridge_t *bridge, const char *name,
                                       rn_native_method_fn handler)
{
    if (!bridge || !name || !handler) return RN_ERR_BAD_ARGS;
    if (bridge->module_count >= RN_BRIDGE_MAX_MODULES) return RN_ERR_QUEUE_FULL;

    for (int i = 0; i < bridge->module_count; i++) {
        if (strcmp(bridge->modules[i].name, name) == 0) {
            bridge->modules[i].handler = handler;
            bridge->modules[i].loaded  = true;
            return RN_OK;
        }
    }

    rn_module_entry_t *entry = &bridge->modules[bridge->module_count++];
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->handler = handler;
    entry->loaded  = true;
    entry->active  = false;

    return RN_OK;
}

static rn_module_entry_t *find_module(rn_bridge_t *bridge, const char *name)
{
    for (int i = 0; i < bridge->module_count; i++) {
        if (strcmp(bridge->modules[i].name, name) == 0) {
            return &bridge->modules[i];
        }
    }
    return NULL;
}

rn_status_t rn_bridge_send_message(rn_bridge_t *bridge, rn_thread_t target,
                                    const char *module, const char *method,
                                    const char *payload)
{
    if (!bridge || !module || !method || !payload) return RN_ERR_BAD_ARGS;
    if (bridge->queue_count >= RN_BRIDGE_QUEUE_SIZE) return RN_ERR_QUEUE_FULL;

    rn_bridge_msg_t *msg = &bridge->queue[bridge->queue_tail];
    msg->id      = bridge->msg_seq++;
    msg->type    = RN_MSG_CALL;
    msg->target  = target;
    strncpy(msg->module, module, sizeof(msg->module) - 1);
    strncpy(msg->method, method, sizeof(msg->method) - 1);
    strncpy(msg->payload, payload, sizeof(msg->payload) - 1);
    msg->user_data = NULL;

    bridge->queue_tail = (bridge->queue_tail + 1) % RN_BRIDGE_QUEUE_SIZE;
    bridge->queue_count++;

    return RN_OK;
}

rn_status_t rn_bridge_send_callback(rn_bridge_t *bridge, int32_t callback_id,
                                     const char *result)
{
    if (!bridge || !result) return RN_ERR_BAD_ARGS;
    if (bridge->queue_count >= RN_BRIDGE_QUEUE_SIZE) return RN_ERR_QUEUE_FULL;

    rn_bridge_msg_t *msg = &bridge->queue[bridge->queue_tail];
    msg->id      = bridge->msg_seq++;
    msg->type    = RN_MSG_CALLBACK;
    msg->target  = RN_THREAD_JS;
    snprintf(msg->module, sizeof(msg->module), "%d", callback_id);
    strncpy(msg->payload, result, sizeof(msg->payload) - 1);
    msg->user_data = NULL;

    bridge->queue_tail = (bridge->queue_tail + 1) % RN_BRIDGE_QUEUE_SIZE;
    bridge->queue_count++;

    return RN_OK;
}

rn_status_t rn_bridge_send_promise(rn_bridge_t *bridge, int32_t callback_id,
                                    const char *result, bool is_error)
{
    if (!bridge || !result) return RN_ERR_BAD_ARGS;
    if (bridge->queue_count >= RN_BRIDGE_QUEUE_SIZE) return RN_ERR_QUEUE_FULL;

    rn_bridge_msg_t *msg = &bridge->queue[bridge->queue_tail];
    msg->id      = bridge->msg_seq++;
    msg->type    = is_error ? RN_MSG_EVENT : RN_MSG_PROMISE;
    msg->target  = RN_THREAD_JS;
    snprintf(msg->module, sizeof(msg->module), "%d", callback_id);
    strncpy(msg->payload, result, sizeof(msg->payload) - 1);
    msg->user_data = NULL;

    bridge->queue_tail = (bridge->queue_tail + 1) % RN_BRIDGE_QUEUE_SIZE;
    bridge->queue_count++;

    return RN_OK;
}

int32_t rn_bridge_register_callback(rn_bridge_t *bridge,
                                     rn_callback_fn resolve,
                                     rn_callback_fn reject,
                                     void *user_data,
                                     int64_t timeout_ms)
{
    if (!bridge) return -1;
    if (bridge->callback_count >= RN_BRIDGE_MAX_CALLBACKS) return -1;

    int idx = bridge->callback_count++;
    rn_bridge_callback_t *cb = &bridge->callbacks[idx];
    cb->id          = bridge->callback_seq++;
    cb->resolve     = resolve;
    cb->reject      = reject;
    cb->user_data   = user_data;
    cb->created_at  = 0;
    cb->timeout_ms  = timeout_ms;

    return cb->id;
}

rn_status_t rn_bridge_flush(rn_bridge_t *bridge, rn_thread_t thread)
{
    if (!bridge) return RN_ERR_BAD_ARGS;

    int flushed = 0;
    while (bridge->queue_count > 0) {
        rn_bridge_msg_t *msg = &bridge->queue[bridge->queue_head];

        if (msg->target != thread) break;

        if (msg->type == RN_MSG_CALL) {
            rn_module_entry_t *mod = find_module(bridge, msg->module);
            if (mod && mod->handler) {
                int32_t cbid = rn_bridge_register_callback(bridge, NULL, NULL, NULL, 5000);
                char cbid_str[32];
                snprintf(cbid_str, sizeof(cbid_str), "{\"cb\":%d}", cbid);
                mod->handler(msg->payload,
                    cbid > 0 ? (rn_callback_fn)(intptr_t)1 : NULL,
                    cbid > 0 ? (rn_callback_fn)(intptr_t)2 : NULL);
            }
        }

        bridge->queue_head = (bridge->queue_head + 1) % RN_BRIDGE_QUEUE_SIZE;
        bridge->queue_count--;
        flushed++;
    }

    (void)flushed;
    return RN_OK;
}

char *rn_bridge_encode_call(const char *module, const char *method,
                             const char *args)
{
    if (!module || !method) return NULL;
    size_t len = strlen(module) + strlen(method) + strlen(args ? args : "") + 64;
    char *buf = (char *)malloc(len);
    if (!buf) return NULL;
    snprintf(buf, len, "{\"module\":\"%s\",\"method\":\"%s\",\"args\":%s}",
             module, method, args ? args : "{}");
    return buf;
}

rn_status_t rn_bridge_decode_call(const char *encoded, char *module_out,
                                   char *method_out, char *args_out,
                                   size_t max_len)
{
    if (!encoded || !module_out || !method_out) return RN_ERR_BAD_ARGS;

    const char *mp = strstr(encoded, "\"module\":\"");
    const char *me = strstr(encoded, "\"method\":\"");
    const char *ap = strstr(encoded, "\"args\":");

    if (!mp || !me) return RN_ERR_METHOD_NF;

    mp += 10;
    const char *ep = strchr(mp, '"');
    if (!ep) return RN_ERR_BAD_ARGS;
    size_t mlen = (size_t)(ep - mp);
    if (mlen >= max_len) mlen = max_len - 1;
    memcpy(module_out, mp, mlen);
    module_out[mlen] = '\0';

    me += 10;
    ep = strchr(me, '"');
    if (!ep) return RN_ERR_BAD_ARGS;
    size_t metlen = (size_t)(ep - me);
    if (metlen >= max_len) metlen = max_len - 1;
    memcpy(method_out, me, metlen);
    method_out[metlen] = '\0';

    if (args_out && ap) {
        ap += 7;
        strncpy(args_out, ap, max_len - 1);
        args_out[max_len - 1] = '\0';
    }

    return RN_OK;
}

int32_t rn_bridge_pending_count(rn_bridge_t *bridge)
{
    return bridge ? bridge->queue_count : 0;
}

void rn_bridge_set_batching(rn_bridge_t *bridge, bool enable)
{
    if (bridge) bridge->batching = enable;
}
