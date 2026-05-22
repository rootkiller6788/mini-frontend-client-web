#ifndef HMR_ENGINE_H
#define HMR_ENGINE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define HMR_MAX_MODULES         512
#define HMR_MAX_DEPENDENCIES    128
#define HMR_MAX_ACCEPTORS       64
#define HMR_SOCKET_BUFFER       8192
#define HMR_URL_LEN             256
#define HMR_UPDATE_QUEUE_SIZE   32

typedef enum {
    HMR_STATE_IDLE,
    HMR_STATE_CHECKING,
    HMR_STATE_READY,
    HMR_STATE_DISPOSING,
    HMR_STATE_APPLYING,
    HMR_STATE_FAILED
} HMRState;

typedef enum {
    HMR_EVENT_CONNECTED,
    HMR_EVENT_DISCONNECTED,
    HMR_EVENT_UPDATE_AVAILABLE,
    HMR_EVENT_UPDATE_APPLIED,
    HMR_EVENT_UPDATE_FAILED,
    HMR_EVENT_FULL_RELOAD
} HMREvent;

typedef struct {
    char module_id[MAX_PATH_LEN];
    bool self_accept;
    bool accept_dispose;
    bool accept_decline;
    int dependency_count;
    char dependency_ids[HMR_MAX_DEPENDENCIES][MAX_PATH_LEN];
    void *module_data;
    size_t module_data_size;
    uint64_t hash;
    uint64_t version;
    bool is_dirty;
} HMRModule;

typedef struct {
    char id[64];
    char module_id[MAX_PATH_LEN];
    char new_source[65536];
    size_t new_source_len;
    char old_source[65536];
    size_t old_source_len;
    uint64_t new_hash;
    char changed_deps[32][MAX_PATH_LEN];
    int changed_dep_count;
    bool is_accepted;
    HMRModule modules_to_update[HMR_MAX_ACCEPTORS];
    int module_count;
    int retry_count;
} HMRUpdate;

typedef struct {
    char boundary_module[MAX_PATH_LEN];
    HMRModule accepted_modules[HMR_MAX_ACCEPTORS];
    int accepted_count;
    char disposed_handlers[32][128];
    int disposed_count;
    bool needs_full_reload;
} HMRBoundary;

typedef struct {
    char ws_url[HMR_URL_LEN];
    bool connected;
    int reconnect_attempts;
    int max_reconnect;
    int retry_interval_ms;
    bool overlay_enabled;
    bool log_enabled;
} HMRClientConfig;

typedef struct {
    HMRModule modules[HMR_MAX_MODULES];
    int module_count;
    HMRUpdate pending_updates[HMR_UPDATE_QUEUE_SIZE];
    int pending_count;
    HMRState state;
    HMRBoundary current_boundary;
    HMRClientConfig config;
    uint64_t global_version;
    int applied_updates;
    int failed_updates;
    char prev_state_snapshots[4][65536];
    int snapshot_count;
} HMREngine;

void hmr_init(HMREngine *e, const char *ws_url);
void hmr_connect(HMREngine *e);
void hmr_disconnect(HMREngine *e);
int  hmr_register_module(HMREngine *e, const char *id, uint64_t hash);
void hmr_add_dependency(HMREngine *e, const char *parent_id, const char *child_id);
void hmr_set_self_accept(HMREngine *e, const char *module_id, bool accept);
int  hmr_handle_update(HMREngine *e, const char *json_payload);
void hmr_apply_updates(HMREngine *e);
void hmr_save_state_snapshot(HMREngine *e);
void hmr_restore_state(HMREngine *e, int snapshot_index);
void hmr_trigger_full_reload(HMREngine *e);
void hmr_send_status(HMREngine *e);
const char* hmr_state_string(HMRState state);

#endif
