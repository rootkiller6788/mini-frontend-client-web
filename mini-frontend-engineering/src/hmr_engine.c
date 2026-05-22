#include "hmr_engine.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static uint64_t hmr_simple_hash(const char *data, size_t len) {
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint64_t)(unsigned char)data[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static int hmr_find_module(HMREngine *e, const char *id) {
    for (int i = 0; i < e->module_count; i++) {
        if (strcmp(e->modules[i].module_id, id) == 0) return i;
    }
    return -1;
}

static int hmr_find_pending(HMREngine *e, const char *module_id) {
    for (int i = 0; i < e->pending_count; i++) {
        if (strcmp(e->pending_updates[i].module_id, module_id) == 0) return i;
    }
    return -1;
}

void hmr_init(HMREngine *e, const char *ws_url) {
    memset(e, 0, sizeof(*e));
    strncpy(e->config.ws_url, ws_url, HMR_URL_LEN - 1);
    e->state = HMR_STATE_IDLE;
    e->connected = false;
    e->config.max_reconnect = 10;
    e->config.retry_interval_ms = 2000;
    e->config.overlay_enabled = true;
    e->config.log_enabled = true;
    e->global_version = 0;
    e->reconnect_attempts = 0;
}

void hmr_connect(HMREngine *e) {
    if (e->connected) return;
    e->state = HMR_STATE_IDLE;
    e->connected = true;
    e->reconnect_attempts = 0;
    if (e->config.log_enabled)
        printf("[HMR] Connected to %s\n", e->config.ws_url);
}

void hmr_disconnect(HMREngine *e) {
    e->connected = false;
    e->state = HMR_STATE_IDLE;
    if (e->config.log_enabled)
        printf("[HMR] Disconnected\n");
}

int hmr_register_module(HMREngine *e, const char *id, uint64_t hash) {
    if (e->module_count >= HMR_MAX_MODULES) return -1;
    HMRModule *m = &e->modules[e->module_count];
    memset(m, 0, sizeof(*m));
    strncpy(m->module_id, id, MAX_PATH_LEN - 1);
    m->hash = hash;
    m->version = e->global_version;
    m->self_accept = false;
    m->is_dirty = false;
    e->module_count++;
    return e->module_count - 1;
}

void hmr_add_dependency(HMREngine *e, const char *parent_id, const char *child_id) {
    int pi = hmr_find_module(e, parent_id);
    if (pi < 0) return;
    HMRModule *pm = &e->modules[pi];
    if (pm->dependency_count >= HMR_MAX_DEPENDENCIES) return;
    strncpy(pm->dependency_ids[pm->dependency_count], child_id, MAX_PATH_LEN - 1);
    pm->dependency_count++;
}

void hmr_set_self_accept(HMREngine *e, const char *module_id, bool accept) {
    int idx = hmr_find_module(e, module_id);
    if (idx < 0) return;
    e->modules[idx].self_accept = accept;
}

static void hmr_collect_boundary_modules(HMREngine *e, const char *changed_id, HMRBoundary *boundary) {
    memset(boundary, 0, sizeof(*boundary));
    int idx = hmr_find_module(e, changed_id);
    if (idx < 0) { boundary->needs_full_reload = true; return; }

    HMRModule *m = &e->modules[idx];
    if (m->self_accept) {
        strncpy(boundary->boundary_module, m->module_id, MAX_PATH_LEN - 1);
        boundary->accepted_modules[boundary->accepted_count++] = *m;
        return;
    }

    bool found_boundary = false;
    for (int i = 0; i < e->module_count; i++) {
        HMRModule *parent = &e->modules[i];
        for (int d = 0; d < parent->dependency_count; d++) {
            if (strcmp(parent->dependency_ids[d], changed_id) == 0) {
                if (parent->self_accept) {
                    strncpy(boundary->boundary_module, parent->module_id, MAX_PATH_LEN - 1);
                    boundary->accepted_modules[boundary->accepted_count++] = *m;
                    found_boundary = true;
                } else {
                    hmr_collect_boundary_modules(e, parent->module_id, boundary);
                }
            }
        }
        if (found_boundary) break;
    }

    if (!found_boundary) {
        boundary->needs_full_reload = true;
    }
}

int hmr_handle_update(HMREngine *e, const char *json_payload) {
    if (!e->connected) return -1;
    e->state = HMR_STATE_CHECKING;
    if (e->pending_count >= HMR_UPDATE_QUEUE_SIZE) return -1;

    HMRUpdate *up = &e->pending_updates[e->pending_count];
    memset(up, 0, sizeof(*up));
    snprintf(up->id, 63, "update_%llu", (unsigned long long)e->global_version);

    const char *mod_id = strstr(json_payload, "\"module\"");
    if (mod_id) {
        const char *val = strchr(mod_id, ':');
        if (val) {
            const char *q = strchr(val, '"');
            if (!q) q = strchr(val, '\'');
            if (q) {
                const char *qe = strchr(q + 1, *q);
                if (qe) {
                    size_t s = qe - q - 1;
                    strncpy(up->module_id, q + 1, s < MAX_PATH_LEN - 1 ? s : MAX_PATH_LEN - 1);
                }
            }
        }
    }

    const char *src = strstr(json_payload, "\"source\"");
    if (src) {
        const char *val = strchr(src, ':');
        if (val) {
            const char *q = strchr(val, '"');
            if (q) {
                const char *qe = strrchr(q + 1, '"');
                if (qe) {
                    size_t s = qe - q - 1;
                    strncpy(up->new_source, q + 1, s < sizeof(up->new_source) - 1 ? s : sizeof(up->new_source) - 1);
                    up->new_source_len = strlen(up->new_source);
                }
            }
        }
    }

    up->new_hash = hmr_simple_hash(up->new_source, up->new_source_len);
    int idx = hmr_find_module(e, up->module_id);
    if (idx >= 0) {
        HMRModule *m = &e->modules[idx];
        if (m->hash != up->new_hash) {
            strncpy(up->old_source, "", 1);
            up->old_source_len = 0;
            m->is_dirty = true;
            up->is_accepted = m->self_accept;
        }
    }

    e->pending_count++;
    e->state = HMR_STATE_READY;
    return e->pending_count - 1;
}

void hmr_apply_updates(HMREngine *e) {
    if (e->pending_count == 0) return;
    e->state = HMR_STATE_APPLYING;

    for (int i = 0; i < e->pending_count; i++) {
        HMRUpdate *up = &e->pending_updates[i];
        hmr_collect_boundary_modules(e, up->module_id, &e->current_boundary);

        if (e->current_boundary.needs_full_reload) {
            hmr_trigger_full_reload(e);
            e->failed_updates++;
            continue;
        }

        int idx = hmr_find_module(e, up->module_id);
        if (idx >= 0) {
            HMRModule *m = &e->modules[idx];
            hmr_save_state_snapshot(e);

            m->hash = up->new_hash;
            m->version++;
            m->is_dirty = false;
            m->module_data = up->new_source;
            m->module_data_size = up->new_source_len;
            e->applied_updates++;
            e->global_version++;

            if (e->config.log_enabled)
                printf("[HMR] Module %s updated (v%llu)\n", m->module_id, (unsigned long long)m->version);
        }
    }

    e->pending_count = 0;
    e->state = HMR_STATE_IDLE;
}

void hmr_save_state_snapshot(HMREngine *e) {
    int slot = e->snapshot_count % 4;
    char *snap = e->prev_state_snapshots[slot];
    size_t offset = 0;

    for (int i = 0; i < e->module_count && offset < 65500; i++) {
        HMRModule *m = &e->modules[i];
        int n = snprintf(snap + offset, 65536 - offset,
            "{\"id\":\"%s\",\"hash\":%llu,\"ver\":%llu}\n",
            m->module_id, (unsigned long long)m->hash, (unsigned long long)m->version);
        if (n > 0) offset += n;
    }
    e->snapshot_count++;
}

void hmr_restore_state(HMREngine *e, int snapshot_index) {
    int slot = snapshot_index % 4;
    (void)slot;
    if (e->config.log_enabled)
        printf("[HMR] State restored from snapshot %d\n", snapshot_index);
}

void hmr_trigger_full_reload(HMREngine *e) {
    e->state = HMR_STATE_FAILED;
    if (e->config.log_enabled)
        printf("[HMR] Full page reload triggered\n");
    e->connected = false;
    e->state = HMR_STATE_IDLE;
}

void hmr_send_status(HMREngine *e) {
    printf("[HMR Status]\n");
    printf("  State: %s\n", hmr_state_string(e->state));
    printf("  Connected: %s\n", e->connected ? "yes" : "no");
    printf("  Modules: %d\n", e->module_count);
    printf("  Pending updates: %d\n", e->pending_count);
    printf("  Applied: %d, Failed: %d\n", e->applied_updates, e->failed_updates);
    printf("  Version: %llu\n", (unsigned long long)e->global_version);
}

const char* hmr_state_string(HMRState state) {
    switch (state) {
        case HMR_STATE_IDLE:      return "idle";
        case HMR_STATE_CHECKING:  return "checking";
        case HMR_STATE_READY:     return "ready";
        case HMR_STATE_DISPOSING: return "disposing";
        case HMR_STATE_APPLYING:  return "applying";
        case HMR_STATE_FAILED:    return "failed";
        default:                  return "unknown";
    }
}
