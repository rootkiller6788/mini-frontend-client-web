#ifndef REACT_NATIVE_BRIDGE_H
#define REACT_NATIVE_BRIDGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RN_BRIDGE_MAX_MODULES   128
#define RN_BRIDGE_MAX_CALLBACKS 64
#define RN_BRIDGE_QUEUE_SIZE    512
#define RN_BRIDGE_MSG_MAX_LEN   4096

typedef enum {
    RN_THREAD_UI = 0,
    RN_THREAD_JS  = 1,
    RN_THREAD_BG  = 2
} rn_thread_t;

typedef enum {
    RN_MSG_CALL     = 0,
    RN_MSG_CALLBACK = 1,
    RN_MSG_EVENT    = 2,
    RN_MSG_PROMISE  = 3
} rn_msg_type_t;

typedef enum {
    RN_OK              = 0,
    RN_ERR_QUEUE_FULL  = -1,
    RN_ERR_MODULE_NF   = -2,
    RN_ERR_METHOD_NF   = -3,
    RN_ERR_BAD_ARGS    = -4,
    RN_ERR_TIMEOUT     = -5
} rn_status_t;

typedef struct rn_bridge_msg {
    int32_t     id;
    rn_msg_type_t type;
    rn_thread_t  target;
    char        module[64];
    char        method[64];
    char        payload[RN_BRIDGE_MSG_MAX_LEN];
    void       *user_data;
} rn_bridge_msg_t;

typedef void (*rn_callback_fn)(const char *result, void *user_data);
typedef void (*rn_native_method_fn)(const char *args, rn_callback_fn resolve, rn_callback_fn reject);

typedef struct rn_module_entry {
    char               name[64];
    rn_native_method_fn handler;
    bool               loaded;
    bool               active;
} rn_module_entry_t;

typedef struct rn_bridge_callback {
    int32_t         id;
    rn_callback_fn  resolve;
    rn_callback_fn  reject;
    void           *user_data;
    int64_t         created_at;
    int64_t         timeout_ms;
} rn_bridge_callback_t;

typedef struct rn_bridge {
    struct rn_bridge_msg *queue;
    int32_t               queue_head;
    int32_t               queue_tail;
    int32_t               queue_count;
    int32_t               msg_seq;

    rn_module_entry_t     modules[RN_BRIDGE_MAX_MODULES];
    int32_t               module_count;

    rn_bridge_callback_t  callbacks[RN_BRIDGE_MAX_CALLBACKS];
    int32_t               callback_count;
    int32_t               callback_seq;

    bool                  batching;
} rn_bridge_t;

rn_status_t  rn_bridge_init        (rn_bridge_t *bridge);
void         rn_bridge_destroy     (rn_bridge_t *bridge);

rn_status_t  rn_bridge_register_module  (rn_bridge_t *bridge, const char *name,
                                         rn_native_method_fn handler);

rn_status_t  rn_bridge_send_message     (rn_bridge_t *bridge, rn_thread_t target,
                                         const char *module, const char *method,
                                         const char *payload);

rn_status_t  rn_bridge_send_callback    (rn_bridge_t *bridge, int32_t callback_id,
                                         const char *result);

rn_status_t  rn_bridge_send_promise     (rn_bridge_t *bridge, int32_t callback_id,
                                         const char *result, bool is_error);

int32_t       rn_bridge_register_callback(rn_bridge_t *bridge,
                                          rn_callback_fn resolve,
                                          rn_callback_fn reject,
                                          void *user_data,
                                          int64_t timeout_ms);

rn_status_t  rn_bridge_flush           (rn_bridge_t *bridge, rn_thread_t thread);

char        *rn_bridge_encode_call     (const char *module, const char *method,
                                        const char *args);
rn_status_t  rn_bridge_decode_call     (const char *encoded, char *module_out,
                                        char *method_out, char *args_out,
                                        size_t max_len);

int32_t       rn_bridge_pending_count  (rn_bridge_t *bridge);
void          rn_bridge_set_batching   (rn_bridge_t *bridge, bool enable);

#ifdef __cplusplus
}
#endif

#endif
