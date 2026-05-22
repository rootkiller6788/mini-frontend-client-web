#include "web_storage.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ================================================================
 *  LocalStorage (persistent per-origin)
 * ================================================================ */

static int wsls_find(const WSLocalStorage *ls, const char *key) {
    int i;
    for (i = 0; i < ls->count; i++)
        if (strcmp(ls->entries[i].key, key) == 0) return i;
    return -1;
}

void ws_localstorage_init(WSLocalStorage *ls, const char *origin,
                          size_t quota_bytes) {
    memset(ls, 0, sizeof(*ls));
    strncpy(ls->origin, origin, WS_KEY_MAX_LEN - 1);
    ls->quota = quota_bytes;
}

void ws_localstorage_destroy(WSLocalStorage *ls) {
    int i;
    for (i = 0; i < ls->count; i++)
        free(ls->entries[i].value);
    memset(ls, 0, sizeof(*ls));
}

int ws_localstorage_set(WSLocalStorage *ls, const char *key,
                         const char *value) {
    int idx = wsls_find(ls, key);
    size_t vlen = strlen(value);
    size_t need;
    if (idx >= 0) {
        need = vlen;
        if (ls->used - strlen(ls->entries[idx].value) + need > ls->quota)
            return 0;
        ls->used -= strlen(ls->entries[idx].value);
        free(ls->entries[idx].value);
    } else {
        need = vlen + sizeof(WSKeyValue);
        if (ls->used + need > ls->quota || ls->count >= WS_MAX_KEYS)
            return 0;
        idx = ls->count++;
    }
    ls->entries[idx].value = (char*)malloc(vlen + 1);
    if (!ls->entries[idx].value) return 0;
    strncpy(ls->entries[idx].key, key, WS_KEY_MAX_LEN - 1);
    memcpy(ls->entries[idx].value, value, vlen + 1);
    ls->entries[idx].value_len = vlen;
    ls->entries[idx].modified = time(NULL);
    if (ls->entries[idx].created == 0)
        ls->entries[idx].created = ls->entries[idx].modified;
    ls->used += need;
    ls->dirty = 1;
    ls->total_sets++;
    return 1;
}

int ws_localstorage_get(const WSLocalStorage *ls, const char *key,
                         char *value_out, size_t max_len) {
    int idx = wsls_find(ls, key);
    if (idx < 0) return 0;
    if (ls->entries[idx].value_len >= max_len) {
        memcpy(value_out, ls->entries[idx].value, max_len - 1);
        value_out[max_len - 1] = '\0';
    } else {
        memcpy(value_out, ls->entries[idx].value, ls->entries[idx].value_len);
        value_out[ls->entries[idx].value_len] = '\0';
    }
    return 1;
}

int ws_localstorage_remove(WSLocalStorage *ls, const char *key) {
    int idx = wsls_find(ls, key);
    if (idx < 0) return 0;
    ls->used -= ls->entries[idx].value_len + sizeof(WSKeyValue);
    free(ls->entries[idx].value);
    if (idx < ls->count - 1)
        memmove(&ls->entries[idx], &ls->entries[idx + 1],
                (size_t)(ls->count - idx - 1) * sizeof(WSKeyValue));
    ls->count--;
    ls->dirty = 1;
    return 1;
}

int ws_localstorage_has(const WSLocalStorage *ls, const char *key) {
    return wsls_find(ls, key) >= 0;
}

void ws_localstorage_clear(WSLocalStorage *ls) {
    int i;
    for (i = 0; i < ls->count; i++)
        free(ls->entries[i].value);
    ls->count = 0;
    ls->used = 0;
    ls->dirty = 1;
}

int ws_localstorage_count(const WSLocalStorage *ls) { return ls->count; }

int ws_localstorage_key(const WSLocalStorage *ls, int index,
                         char *key_out, size_t max_len) {
    if (index < 0 || index >= ls->count) return 0;
    strncpy(key_out, ls->entries[index].key, max_len - 1);
    key_out[max_len - 1] = '\0';
    return 1;
}

size_t ws_localstorage_remaining(const WSLocalStorage *ls) {
    if (ls->quota <= ls->used) return 0;
    return ls->quota - ls->used;
}

int ws_localstorage_save(const WSLocalStorage *ls, const char *filepath) {
    FILE *fp = fopen(filepath, "wb");
    int i;
    if (!fp) return 0;
    fprintf(fp, "origin=%s\n", ls->origin);
    fprintf(fp, "quota=%zu\n", ls->quota);
    for (i = 0; i < ls->count; i++) {
        fprintf(fp, "K:%s\nV:%s\n", ls->entries[i].key, ls->entries[i].value);
    }
    fclose(fp);
    return 1;
}

int ws_localstorage_load(WSLocalStorage *ls, const char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    char line[WS_VALUE_MAX_LEN];
    char key[WS_KEY_MAX_LEN];
    char val[WS_VALUE_MAX_LEN];
    if (!fp) return 0;
    ws_localstorage_clear(ls);
    while (fgets(line, (int)sizeof(line), fp)) {
        if (strncmp(line, "origin=", 7) == 0) {
            char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
            strncpy(ls->origin, line + 7, WS_KEY_MAX_LEN - 1);
        } else if (strncmp(line, "quota=", 6) == 0) {
            ls->quota = (size_t)strtoull(line + 6, NULL, 10);
        } else if (strncmp(line, "K:", 2) == 0) {
            char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
            strncpy(key, line + 2, WS_KEY_MAX_LEN - 1);
        } else if (strncmp(line, "V:", 2) == 0) {
            char *nl = strchr(line, '\n'); if (nl) *nl = '\0';
            strncpy(val, line + 2, WS_VALUE_MAX_LEN - 1);
            ws_localstorage_set(ls, key, val);
        }
    }
    fclose(fp);
    return 1;
}

/* ================================================================
 *  SessionStorage (per-tab)
 * ================================================================ */

static int wsss_find(const WSSessionStorage *ss, const char *key) {
    int i;
    for (i = 0; i < ss->count; i++)
        if (strcmp(ss->entries[i].key, key) == 0) return i;
    return -1;
}

void ws_sessionstorage_init(WSSessionStorage *ss, const char *tab_id,
                            const char *origin, size_t quota_bytes) {
    memset(ss, 0, sizeof(*ss));
    strncpy(ss->tab_id, tab_id, WS_KEY_MAX_LEN - 1);
    strncpy(ss->origin, origin, WS_KEY_MAX_LEN - 1);
    ss->quota = quota_bytes;
}

void ws_sessionstorage_destroy(WSSessionStorage *ss) {
    int i;
    for (i = 0; i < ss->count; i++)
        free(ss->entries[i].value);
    memset(ss, 0, sizeof(*ss));
}

int ws_sessionstorage_set(WSSessionStorage *ss, const char *key,
                           const char *value) {
    int idx = wsss_find(ss, key);
    size_t vlen = strlen(value);
    size_t need;
    if (idx >= 0) {
        need = vlen;
        if (ss->used - strlen(ss->entries[idx].value) + need > ss->quota)
            return 0;
        ss->used -= strlen(ss->entries[idx].value);
        free(ss->entries[idx].value);
    } else {
        need = vlen + sizeof(WSKeyValue);
        if (ss->used + need > ss->quota || ss->count >= WS_MAX_KEYS)
            return 0;
        idx = ss->count++;
    }
    ss->entries[idx].value = (char*)malloc(vlen + 1);
    if (!ss->entries[idx].value) return 0;
    strncpy(ss->entries[idx].key, key, WS_KEY_MAX_LEN - 1);
    memcpy(ss->entries[idx].value, value, vlen + 1);
    ss->entries[idx].value_len = vlen;
    ss->used += need;
    return 1;
}

int ws_sessionstorage_get(const WSSessionStorage *ss, const char *key,
                           char *value_out, size_t max_len) {
    int idx = wsss_find(ss, key);
    if (idx < 0) return 0;
    if (ss->entries[idx].value_len >= max_len) {
        memcpy(value_out, ss->entries[idx].value, max_len - 1);
        value_out[max_len - 1] = '\0';
    } else {
        memcpy(value_out, ss->entries[idx].value, ss->entries[idx].value_len);
        value_out[ss->entries[idx].value_len] = '\0';
    }
    return 1;
}

int ws_sessionstorage_remove(WSSessionStorage *ss, const char *key) {
    int idx = wsss_find(ss, key);
    if (idx < 0) return 0;
    ss->used -= ss->entries[idx].value_len + sizeof(WSKeyValue);
    free(ss->entries[idx].value);
    if (idx < ss->count - 1)
        memmove(&ss->entries[idx], &ss->entries[idx + 1],
                (size_t)(ss->count - idx - 1) * sizeof(WSKeyValue));
    ss->count--;
    return 1;
}

int ws_sessionstorage_has(const WSSessionStorage *ss, const char *key) {
    return wsss_find(ss, key) >= 0;
}

void ws_sessionstorage_clear(WSSessionStorage *ss) {
    int i;
    for (i = 0; i < ss->count; i++) free(ss->entries[i].value);
    ss->count = 0;
    ss->used = 0;
}

int ws_sessionstorage_count(const WSSessionStorage *ss) { return ss->count; }

/* ================================================================
 *  StorageHub
 * ================================================================ */

void storage_hub_init(StorageHub *hub, const char *origin,
                      size_t local_quota, size_t session_quota) {
    memset(hub, 0, sizeof(*hub));
    ws_localstorage_init(&hub->localstorage, origin, local_quota);
    hub->session_count = 0;
}

void storage_hub_destroy(StorageHub *hub) {
    int i;
    ws_localstorage_destroy(&hub->localstorage);
    for (i = 0; i < hub->session_count; i++)
        ws_sessionstorage_destroy(&hub->sessions[i]);
    memset(hub, 0, sizeof(*hub));
}

int storage_hub_open_tab(StorageHub *hub, const char *tab_id) {
    if (hub->session_count >= WS_MAX_TABS) return 0;
    ws_sessionstorage_init(&hub->sessions[hub->session_count++],
                           tab_id, hub->localstorage.origin,
                           (size_t)(5 * 1024 * 1024)); /* 5MB default */
    return 1;
}

int storage_hub_close_tab(StorageHub *hub, const char *tab_id) {
    int i;
    for (i = 0; i < hub->session_count; i++) {
        if (strcmp(hub->sessions[i].tab_id, tab_id) == 0) {
            ws_sessionstorage_destroy(&hub->sessions[i]);
            if (i < hub->session_count - 1)
                memmove(&hub->sessions[i], &hub->sessions[i + 1],
                        (size_t)(hub->session_count - i - 1) * sizeof(WSSessionStorage));
            hub->session_count--;
            return 1;
        }
    }
    return 0;
}

int storage_hub_local_set(StorageHub *hub, const char *key,
                           const char *value) {
    StorageEvent ev;
    char old_val[WS_VALUE_MAX_LEN] = "";
    int had_old = ws_localstorage_get(&hub->localstorage, key,
                                       old_val, sizeof(old_val));
    int result = ws_localstorage_set(&hub->localstorage, key, value);
    if (result) {
        memset(&ev, 0, sizeof(ev));
        ev.event_type = had_old ? STORAGE_EVENT_SET : STORAGE_EVENT_SET;
        strncpy(ev.origin, hub->localstorage.origin, WS_KEY_MAX_LEN - 1);
        strncpy(ev.key, key, WS_KEY_MAX_LEN - 1);
        if (had_old) strncpy(ev.old_value, old_val, WS_VALUE_MAX_LEN - 1);
        strncpy(ev.new_value, value, WS_VALUE_MAX_LEN - 1);
        ev.is_local_storage = 1;
        ev.timestamp = time(NULL);
        storage_hub_fire_event(hub, &ev);
    }
    return result;
}

int storage_hub_local_get(StorageHub *hub, const char *key,
                           char *value_out, size_t max_len) {
    return ws_localstorage_get(&hub->localstorage, key, value_out, max_len);
}

int storage_hub_local_remove(StorageHub *hub, const char *key) {
    StorageEvent ev;
    char old_val[WS_VALUE_MAX_LEN] = "";
    int had_old = ws_localstorage_get(&hub->localstorage, key,
                                       old_val, sizeof(old_val));
    int result = ws_localstorage_remove(&hub->localstorage, key);
    if (result && had_old) {
        memset(&ev, 0, sizeof(ev));
        ev.event_type = STORAGE_EVENT_REMOVE;
        strncpy(ev.origin, hub->localstorage.origin, WS_KEY_MAX_LEN - 1);
        strncpy(ev.key, key, WS_KEY_MAX_LEN - 1);
        strncpy(ev.old_value, old_val, WS_VALUE_MAX_LEN - 1);
        ev.is_local_storage = 1;
        ev.timestamp = time(NULL);
        storage_hub_fire_event(hub, &ev);
    }
    return result;
}

void storage_hub_local_clear(StorageHub *hub) {
    StorageEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.event_type = STORAGE_EVENT_CLEAR;
    strncpy(ev.origin, hub->localstorage.origin, WS_KEY_MAX_LEN - 1);
    ev.is_local_storage = 1;
    ev.timestamp = time(NULL);
    ws_localstorage_clear(&hub->localstorage);
    storage_hub_fire_event(hub, &ev);
}

int storage_hub_session_set(StorageHub *hub, const char *tab_id,
                             const char *key, const char *value) {
    int i;
    for (i = 0; i < hub->session_count; i++) {
        if (strcmp(hub->sessions[i].tab_id, tab_id) == 0)
            return ws_sessionstorage_set(&hub->sessions[i], key, value);
    }
    return 0;
}

int storage_hub_session_get(StorageHub *hub, const char *tab_id,
                             const char *key, char *value_out, size_t max_len) {
    int i;
    for (i = 0; i < hub->session_count; i++) {
        if (strcmp(hub->sessions[i].tab_id, tab_id) == 0)
            return ws_sessionstorage_get(&hub->sessions[i], key, value_out, max_len);
    }
    return 0;
}

void storage_hub_on_storage(StorageHub *hub,
                            StorageEventListener listener,
                            void *user_data) {
    hub->storage_listener = listener;
    hub->storage_listener_data = user_data;
}

void storage_hub_fire_event(StorageHub *hub, const StorageEvent *event) {
    if (hub->storage_listener)
        hub->storage_listener(event, hub->storage_listener_data);
}

/* ================================================================
 *  IndexedDB simulation
 * ================================================================ */

void idb_database_init(IDBDatabase *db, const char *name, int version) {
    memset(db, 0, sizeof(*db));
    strncpy(db->db_name, name, WS_KEY_MAX_LEN - 1);
    db->version = version;
}

void idb_database_destroy(IDBDatabase *db) {
    int i, j;
    for (i = 0; i < db->store_count; i++) {
        IDBObjectStore *store = &db->stores[i];
        for (j = 0; j < store->record_count; j++)
            free(store->records[j].value);
        for (j = 0; j < store->index_count; j++)
            free(store->indexes[j].sorted_indices);
    }
    memset(db, 0, sizeof(*db));
}

int idb_create_store(IDBDatabase *db, const char *name,
                     const char *key_path, int auto_increment) {
    if (db->store_count >= IDB_MAX_STORES) return 0;
    {
        IDBObjectStore *store = &db->stores[db->store_count++];
        memset(store, 0, sizeof(*store));
        strncpy(store->name, name, WS_STORE_NAME_LEN - 1);
        if (key_path)
            strncpy(store->key_path, key_path, WS_KEY_MAX_LEN - 1);
        store->auto_increment = auto_increment;
        store->next_key = 1;
    }
    return 1;
}

IDBObjectStore *idb_get_store(IDBDatabase *db, const char *name) {
    int i;
    for (i = 0; i < db->store_count; i++)
        if (strcmp(db->stores[i].name, name) == 0)
            return &db->stores[i];
    return NULL;
}

int idb_delete_store(IDBDatabase *db, const char *name) {
    int i;
    for (i = 0; i < db->store_count; i++) {
        if (strcmp(db->stores[i].name, name) == 0) {
            IDBObjectStore *store = &db->stores[i];
            int j;
            for (j = 0; j < store->record_count; j++)
                free(store->records[j].value);
            for (j = 0; j < store->index_count; j++)
                free(store->indexes[j].sorted_indices);
            if (i < db->store_count - 1)
                memmove(&db->stores[i], &db->stores[i + 1],
                        (size_t)(db->store_count - i - 1) * sizeof(IDBObjectStore));
            db->store_count--;
            return 1;
        }
    }
    return 0;
}

static int idb_store_find(const IDBObjectStore *store, const char *key) {
    int i;
    for (i = 0; i < store->record_count; i++)
        if (strcmp(store->records[i].key, key) == 0) return i;
    return -1;
}

int idb_store_put(IDBObjectStore *store, const char *key,
                  const char *value, size_t value_len) {
    int idx;
    char use_key[IDB_RECORD_KEY_LEN];

    if (key && key[0]) {
        strncpy(use_key, key, IDB_RECORD_KEY_LEN - 1);
    } else if (store->auto_increment) {
        snprintf(use_key, sizeof(use_key), "%d", store->next_key++);
    } else {
        return 0;
    }

    idx = idb_store_find(store, use_key);
    if (idx >= 0) {
        free(store->records[idx].value);
    } else {
        if (store->record_count >= IDB_MAX_RECORDS) return 0;
        idx = store->record_count++;
    }

    strncpy(store->records[idx].key, use_key, IDB_RECORD_KEY_LEN - 1);
    store->records[idx].value = (char*)malloc(value_len + 1);
    if (!store->records[idx].value) return 0;
    memcpy(store->records[idx].value, value, value_len);
    store->records[idx].value[value_len] = '\0';
    store->records[idx].value_len = value_len;

    idb_store_rebuild_indexes(store);
    return 1;
}

int idb_store_get(const IDBObjectStore *store, const char *key,
                   char *value_out, size_t *value_len_out) {
    int idx = idb_store_find(store, key);
    if (idx < 0) return 0;
    if (value_out && value_len_out) {
        size_t copy = (store->records[idx].value_len < *value_len_out)
                      ? store->records[idx].value_len : *value_len_out;
        memcpy(value_out, store->records[idx].value, copy);
        *value_len_out = store->records[idx].value_len;
    }
    return 1;
}

int idb_store_delete(IDBObjectStore *store, const char *key) {
    int idx = idb_store_find(store, key);
    if (idx < 0) return 0;
    free(store->records[idx].value);
    if (idx < store->record_count - 1)
        memmove(&store->records[idx], &store->records[idx + 1],
                (size_t)(store->record_count - idx - 1) * sizeof(IDBRecord));
    store->record_count--;
    idb_store_rebuild_indexes(store);
    return 1;
}

int idb_store_count(const IDBObjectStore *store) {
    return store->record_count;
}

void idb_store_clear(IDBObjectStore *store) {
    int i;
    for (i = 0; i < store->record_count; i++)
        free(store->records[i].value);
    store->record_count = 0;
    store->next_key = 1;
    for (i = 0; i < store->index_count; i++) {
        free(store->indexes[i].sorted_indices);
        store->indexes[i].sorted_indices = NULL;
        store->indexes[i].count = 0;
    }
}

int idb_create_index(IDBObjectStore *store, const char *name,
                      const char *key_path, int unique) {
    (void)key_path;
    if (store->index_count >= IDB_MAX_INDEXES) return 0;
    {
        IDBIndex *idx = &store->indexes[store->index_count++];
        memset(idx, 0, sizeof(*idx));
        strncpy(idx->name, name, WS_INDEX_NAME_LEN - 1);
        idx->unique = unique;
    }
    idb_store_rebuild_indexes(store);
    return 1;
}

void idb_store_rebuild_indexes(IDBObjectStore *store) {
    int i;
    for (i = 0; i < store->index_count; i++) {
        IDBIndex *idx = &store->indexes[i];
        free(idx->sorted_indices);
        idx->sorted_indices = (int*)malloc(
            (size_t)store->record_count * sizeof(int));
        if (!idx->sorted_indices) { idx->count = 0; continue; }
        idx->count = store->record_count;
        for (i = 0; i < store->record_count; i++)
            idx->sorted_indices[i] = i;
    }
}

void idb_cursor_open(IDBCursor *cursor, IDBDatabase *db,
                      const char *store_name, const char *index_name,
                      IDBCursorDirection direction) {
    memset(cursor, 0, sizeof(*cursor));
    cursor->db = db;
    cursor->store = idb_get_store(db, store_name);
    cursor->index = NULL;
    cursor->position = 0;
    cursor->direction = direction;

    if (index_name && cursor->store) {
        int i;
        for (i = 0; i < cursor->store->index_count; i++) {
            if (strcmp(cursor->store->indexes[i].name, index_name) == 0) {
                cursor->index = &cursor->store->indexes[i];
                break;
            }
        }
    }

    if (cursor->store && cursor->store->record_count > 0) {
        int pos = (direction == CURSOR_PREV || direction == CURSOR_PREV_UNIQUE)
                  ? cursor->store->record_count - 1 : 0;
        cursor->position = pos;
        cursor->has_value = 1;
    }
}

int idb_cursor_next(IDBCursor *cursor) {
    if (!cursor->store || cursor->store->record_count == 0) {
        cursor->has_value = 0;
        return 0;
    }

    if (cursor->direction == CURSOR_NEXT || cursor->direction == CURSOR_NEXT_UNIQUE) {
        cursor->position++;
        if (cursor->position >= cursor->store->record_count) {
            cursor->has_value = 0;
            return 0;
        }
    } else {
        cursor->position--;
        if (cursor->position < 0) {
            cursor->has_value = 0;
            return 0;
        }
    }

    cursor->has_value = 1;
    return 1;
}

int idb_cursor_valid(const IDBCursor *cursor) {
    return cursor->has_value && cursor->store
           && cursor->position >= 0
           && cursor->position < cursor->store->record_count;
}

void idb_cursor_value(const IDBCursor *cursor, char *value_out,
                       size_t *value_len_out) {
    if (!idb_cursor_valid(cursor)) { *value_len_out = 0; return; }
    {
        int pos = cursor->position;
        if (cursor->index && cursor->index->sorted_indices)
            pos = cursor->index->sorted_indices[cursor->position];
        if (cursor->store->records[pos].value_len < *value_len_out) {
            memcpy(value_out, cursor->store->records[pos].value,
                   cursor->store->records[pos].value_len);
            *value_len_out = cursor->store->records[pos].value_len;
        } else {
            *value_len_out = cursor->store->records[pos].value_len;
        }
    }
}

void idb_cursor_key(const IDBCursor *cursor, char *key_out, size_t max_len) {
    if (!idb_cursor_valid(cursor)) { key_out[0] = '\0'; return; }
    {
        int pos = cursor->position;
        if (cursor->index && cursor->index->sorted_indices)
            pos = cursor->index->sorted_indices[cursor->position];
        strncpy(key_out, cursor->store->records[pos].key, max_len - 1);
        key_out[max_len - 1] = '\0';
    }
}
