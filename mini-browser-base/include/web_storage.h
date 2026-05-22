#ifndef WEB_STORAGE_H
#define WEB_STORAGE_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#define WS_KEY_MAX_LEN        256
#define WS_VALUE_MAX_LEN      65536
#define WS_MAX_KEYS           2048
#define WS_MAX_TABS           64
#define WS_MAX_INDEXES        32
#define WS_INDEX_NAME_LEN     64
#define WS_STORE_NAME_LEN     64
#define WS_CURSOR_BATCH_SIZE  16

/* ================================================================
 *  LocalStorage — persistent per-origin key-value store
 * ================================================================ */
typedef struct {
    char   key[WS_KEY_MAX_LEN];
    char  *value;
    size_t value_len;
    time_t created;
    time_t modified;
} WSKeyValue;

typedef struct {
    WSKeyValue entries[WS_MAX_KEYS];
    int        count;
    char       origin[WS_KEY_MAX_LEN];
    size_t     quota;
    size_t     used;
    int        dirty;
    uint64_t   total_sets;
    uint64_t   total_gets;
} WSLocalStorage;

void   ws_localstorage_init(WSLocalStorage *ls, const char *origin,
                            size_t quota_bytes);
void   ws_localstorage_destroy(WSLocalStorage *ls);

int    ws_localstorage_set(WSLocalStorage *ls, const char *key,
                           const char *value);
int    ws_localstorage_get(const WSLocalStorage *ls, const char *key,
                           char *value_out, size_t max_len);
int    ws_localstorage_remove(WSLocalStorage *ls, const char *key);
int    ws_localstorage_has(const WSLocalStorage *ls, const char *key);
void   ws_localstorage_clear(WSLocalStorage *ls);
int    ws_localstorage_count(const WSLocalStorage *ls);
int    ws_localstorage_key(const WSLocalStorage *ls, int index,
                           char *key_out, size_t max_len);
size_t ws_localstorage_remaining(const WSLocalStorage *ls);

/* persistence */
int    ws_localstorage_save(const WSLocalStorage *ls, const char *filepath);
int    ws_localstorage_load(WSLocalStorage *ls, const char *filepath);

/* ================================================================
 *  SessionStorage — per-tab key-value store
 * ================================================================ */
typedef struct {
    char       tab_id[WS_KEY_MAX_LEN];
    WSKeyValue entries[WS_MAX_KEYS];
    int        count;
    char       origin[WS_KEY_MAX_LEN];
    size_t     quota;
    size_t     used;
} WSSessionStorage;

void   ws_sessionstorage_init(WSSessionStorage *ss, const char *tab_id,
                              const char *origin, size_t quota_bytes);
void   ws_sessionstorage_destroy(WSSessionStorage *ss);

int    ws_sessionstorage_set(WSSessionStorage *ss, const char *key,
                             const char *value);
int    ws_sessionstorage_get(const WSSessionStorage *ss, const char *key,
                             char *value_out, size_t max_len);
int    ws_sessionstorage_remove(WSSessionStorage *ss, const char *key);
int    ws_sessionstorage_has(const WSSessionStorage *ss, const char *key);
void   ws_sessionstorage_clear(WSSessionStorage *ss);
int    ws_sessionstorage_count(const WSSessionStorage *ss);

/* ================================================================
 *  StorageEvent — cross-tab notification
 * ================================================================ */
typedef enum {
    STORAGE_EVENT_SET    = 0,
    STORAGE_EVENT_REMOVE = 1,
    STORAGE_EVENT_CLEAR  = 2
} StorageEventType;

typedef struct {
    StorageEventType event_type;
    char             origin[WS_KEY_MAX_LEN];
    char             key[WS_KEY_MAX_LEN];
    char             old_value[WS_VALUE_MAX_LEN];
    char             new_value[WS_VALUE_MAX_LEN];
    int              is_local_storage;
    time_t           timestamp;
} StorageEvent;

typedef void (*StorageEventListener)(const StorageEvent *event,
                                     void *user_data);

/* ================================================================
 *  StorageHub — manages multiple tabs + cross-tab events
 * ================================================================ */
typedef struct {
    WSLocalStorage       localstorage;
    WSSessionStorage     sessions[WS_MAX_TABS];
    int                  session_count;

    StorageEventListener storage_listener;
    void                *storage_listener_data;
} StorageHub;

void storage_hub_init(StorageHub *hub, const char *origin,
                      size_t local_quota, size_t session_quota);
void storage_hub_destroy(StorageHub *hub);

int  storage_hub_open_tab(StorageHub *hub, const char *tab_id);
int  storage_hub_close_tab(StorageHub *hub, const char *tab_id);

int  storage_hub_local_set(StorageHub *hub, const char *key,
                           const char *value);
int  storage_hub_local_get(StorageHub *hub, const char *key,
                           char *value_out, size_t max_len);
int  storage_hub_local_remove(StorageHub *hub, const char *key);
void storage_hub_local_clear(StorageHub *hub);

int  storage_hub_session_set(StorageHub *hub, const char *tab_id,
                             const char *key, const char *value);
int  storage_hub_session_get(StorageHub *hub, const char *tab_id,
                             const char *key, char *value_out, size_t max_len);

void storage_hub_on_storage(StorageHub *hub,
                            StorageEventListener listener,
                            void *user_data);

void storage_hub_fire_event(StorageHub *hub,
                            const StorageEvent *event);

/* ================================================================
 *  IndexedDB simulation — object store, index, cursor
 * ================================================================ */
#define IDB_MAX_STORES         16
#define IDB_MAX_RECORDS        4096
#define IDB_RECORD_KEY_LEN     256
#define IDB_RECORD_VALUE_LEN   65536
#define IDB_MAX_INDEXES        32

typedef struct {
    char   key[IDB_RECORD_KEY_LEN];
    char  *value;
    size_t value_len;
} IDBRecord;

typedef struct {
    char   name[WS_INDEX_NAME_LEN];
    int   *sorted_indices;
    int    count;
    int    unique;
} IDBIndex;

typedef struct {
    char       name[WS_STORE_NAME_LEN];
    IDBRecord  records[IDB_MAX_RECORDS];
    int        record_count;
    char       key_path[WS_KEY_MAX_LEN];
    int        auto_increment;
    int        next_key;

    IDBIndex   indexes[IDB_MAX_INDEXES];
    int        index_count;
} IDBObjectStore;

typedef struct {
    IDBObjectStore stores[IDB_MAX_STORES];
    int            store_count;
    char           db_name[WS_KEY_MAX_LEN];
    int            version;
} IDBDatabase;

/* Cursor types */
typedef enum {
    CURSOR_NEXT        = 0,
    CURSOR_NEXT_UNIQUE = 1,
    CURSOR_PREV        = 2,
    CURSOR_PREV_UNIQUE = 3
} IDBCursorDirection;

typedef struct {
    IDBDatabase     *db;
    IDBObjectStore  *store;
    IDBIndex        *index;
    int              position;
    int              has_value;
    char             current_key[IDB_RECORD_KEY_LEN];
    char             current_value[IDB_RECORD_VALUE_LEN];
    IDBCursorDirection direction;
} IDBCursor;

void idb_database_init(IDBDatabase *db, const char *name, int version);
void idb_database_destroy(IDBDatabase *db);

int  idb_create_store(IDBDatabase *db, const char *name,
                      const char *key_path, int auto_increment);
IDBObjectStore *idb_get_store(IDBDatabase *db, const char *name);
int  idb_delete_store(IDBDatabase *db, const char *name);

int  idb_store_put(IDBObjectStore *store, const char *key,
                   const char *value, size_t value_len);
int  idb_store_get(const IDBObjectStore *store, const char *key,
                   char *value_out, size_t *value_len_out);
int  idb_store_delete(IDBObjectStore *store, const char *key);
int  idb_store_count(const IDBObjectStore *store);
void idb_store_clear(IDBObjectStore *store);

int  idb_create_index(IDBObjectStore *store, const char *name,
                      const char *key_path, int unique);
void idb_store_rebuild_indexes(IDBObjectStore *store);

void idb_cursor_open(IDBCursor *cursor, IDBDatabase *db,
                     const char *store_name, const char *index_name,
                     IDBCursorDirection direction);
int  idb_cursor_next(IDBCursor *cursor);
int  idb_cursor_valid(const IDBCursor *cursor);
void idb_cursor_value(const IDBCursor *cursor, char *value_out,
                      size_t *value_len_out);
void idb_cursor_key(const IDBCursor *cursor, char *key_out,
                    size_t max_len);

#endif
