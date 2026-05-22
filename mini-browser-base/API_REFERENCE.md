# mini-browser-base — API Reference

## Modules

| Module | Header | Source | Description |
|--------|--------|--------|-------------|
| HTTP Cache | `http_cache.h` | `http_cache.c` | LRU cache with ETag/304 conditional revalidation, Cache-Control directives |
| Cookie Storage | `cookie_storage.h` | `cookie_storage.c` | Cookie jar, Set-Cookie parsing, domain/path matching, localStorage, sessionStorage |
| Event System | `event_system.h` | `event_system.c` | DOM events with capture→target→bubble phases, addEventListener/removeEventListener |
| Fetch Model | `fetch_model.h` | `fetch_model.c` | Fetch API model (Request/Response), AbortController, CORS, interceptors |
| Web Storage | `web_storage.h` | `web_storage.c` | localStorage, sessionStorage, StorageEvent, IndexedDB simulation |

---

## http_cache.h

### Types

- `CacheDirective` — bitmask of `CACHE_DIRECTIVE_MAX_AGE`, `CACHE_DIRECTIVE_NO_CACHE`, `CACHE_DIRECTIVE_NO_STORE`, `CACHE_DIRECTIVE_STALE_WHILE_REV`, `CACHE_DIRECTIVE_PUBLIC`, `CACHE_DIRECTIVE_PRIVATE`, `CACHE_DIRECTIVE_MUST_REVALIDATE`, `CACHE_DIRECTIVE_IMMUTABLE`
- `CacheHeader` — `{ name[256], value[256] }`
- `CacheEntry` — single cache entry with key, value, ETag, Last-Modified, headers, directives, timestamps
- `HttpCache` — the cache: entries array, hit/miss/eviction/revalidation counters, size tracking

### Functions

```
void cache_init(HttpCache *cache, size_t max_size);
void cache_destroy(HttpCache *cache);

int  cache_store(HttpCache *cache, const char *key, const char *value,
                 size_t value_len, const CacheHeader *headers, int header_count);

int  cache_lookup(HttpCache *cache, const char *key, char *value_out,
                  size_t *value_len_out, CacheHeader *headers_out,
                  int *header_count_out);

int  cache_needs_revalidation(const HttpCache *cache, const char *key);
void cache_prepare_conditional(HttpCache *cache, const char *key,
                               const char **if_none_match,
                               const char **if_modified_since);
int  cache_update_with_304(HttpCache *cache, const char *key,
                           const CacheHeader *headers, int header_count);

int  cache_invalidate(HttpCache *cache, const char *key);
int  cache_invalidate_prefix(HttpCache *cache, const char *prefix);
void cache_clear(HttpCache *cache);

void cache_parse_cache_control(CacheEntry *entry, const char *header_value);
int  cache_is_fresh(const CacheEntry *entry);

void   cache_get_stats(const HttpCache *cache, ...);
double cache_hit_ratio(const HttpCache *cache);
```

---

## cookie_storage.h

### Types

- `CookieSameSite` — `COOKIE_SAMESITE_NONE`, `COOKIE_SAMESITE_LAX`, `COOKIE_SAMESITE_STRICT`
- `Cookie` — name, value, domain, path, expires, http_only, secure, same_site, host_only
- `CookieJar` — array of cookies, origin domain
- `LocalStorage` — per-origin persistent key-value store
- `SessionStorage` — per-tab key-value store

### CookieJar Functions

```
void cookie_jar_init(CookieJar *jar, const char *origin);
int  cookie_parse_set_cookie(Cookie *cookie, const char *header_val, ...);
int  cookie_jar_insert(CookieJar *jar, const Cookie *cookie);
int  cookie_jar_remove(CookieJar *jar, const char *name, ...);
int  cookie_jar_match(const CookieJar *jar, const char *domain, ...);
int  cookie_jar_serialize_for_request(const CookieJar *jar, ...);
void cookie_jar_purge_expired(CookieJar *jar);
int  cookie_domain_match(const char *cookie_domain, const char *request_domain);
int  cookie_path_match(const char *cookie_path, const char *request_path);
```

### LocalStorage Functions

```
void localStorage_init(LocalStorage *ls, const char *origin, size_t quota);
int  localStorage_set(LocalStorage *ls, const char *key, const char *value);
int  localStorage_get(LocalStorage *ls, const char *key, char *value_out, size_t max);
int  localStorage_remove(LocalStorage *ls, const char *key);
int  localStorage_has(LocalStorage *ls, const char *key);
void localStorage_clear(LocalStorage *ls);
int  localStorage_count(const LocalStorage *ls);
int  localStorage_key_at(const LocalStorage *ls, int idx, char *out, size_t max);
```

### SessionStorage Functions

```
void sessionStorage_init(SessionStorage *ss, const char *origin, size_t quota);
int  sessionStorage_set(SessionStorage *ss, const char *key, const char *value);
int  sessionStorage_get(SessionStorage *ss, const char *key, char *out, size_t max);
int  sessionStorage_remove(SessionStorage *ss, const char *key);
int  sessionStorage_has(SessionStorage *ss, const char *key);
void sessionStorage_clear(SessionStorage *ss);
int  sessionStorage_count(const SessionStorage *ss);
int  sessionStorage_key_at(const SessionStorage *ss, int idx, char *out, size_t max);
```

---

## event_system.h

### Types

- `EventPhase` — `EVENT_PHASE_NONE`, `EVENT_PHASE_CAPTURING`, `EVENT_PHASE_AT_TARGET`, `EVENT_PHASE_BUBBLING`
- `EventFlag` — `EVENT_FLAG_BUBBLES`, `EVENT_FLAG_CANCELABLE`, `EVENT_FLAG_COMPOSED`, `EVENT_FLAG_TRUSTED`
- `Event` — type, target, currentTarget, phase, flags, timestamp, data, cancellation state
- `EventListener` — function pointer `void (*)(const Event*, void*)`
- `EventListenerEntry` — type, callback, capture flag, once, passive
- `EventTarget` — listeners array, id, parent/children tree, user_data

### Functions

```
void event_init(Event *event, const char *type, uint32_t flags);
void event_set_data(Event *event, const char *data, int len);
void event_prevent_default(Event *event);
void event_stop_propagation(Event *event);
void event_stop_immediate_propagation(Event *event);

void event_target_init(EventTarget *target, const char *id);
void event_target_destroy(EventTarget *target);
void event_target_append_child(EventTarget *parent, EventTarget *child);
void event_target_remove_child(EventTarget *parent, EventTarget *child);

int event_add_listener(EventTarget *target, const char *type,
       EventListener cb, void *user_data, int use_capture, int once, int passive);
int event_remove_listener(EventTarget *target, const char *type,
       EventListener cb, int use_capture);

int event_dispatch(EventTarget *target, Event *event);
int event_dispatch_at(EventTarget *target, const char *type,
       uint32_t flags, const char *data, int data_len);
void event_build_propagation_path(EventTarget *target,
       EventTarget **path, int *path_len);

int event_has_flag(const Event *event, uint32_t flag);
int event_type_is(const Event *event, const char *type);
```

---

## fetch_model.h

### Types

- `FetchMode` — `FETCH_MODE_SAME_ORIGIN`, `FETCH_MODE_CORS`, `FETCH_MODE_NO_CORS`, `FETCH_MODE_NAVIGATE`
- `FetchCredentials` — `FETCH_CREDENTIALS_OMIT`, `FETCH_CREDENTIALS_SAME_ORIGIN`, `FETCH_CREDENTIALS_INCLUDE`
- `FetchRedirect` — `FETCH_REDIRECT_FOLLOW`, `FETCH_REDIRECT_ERROR`, `FETCH_REDIRECT_MANUAL`
- `FetchHeader` — `{ name[128], value[1024] }`
- `FetchRequest` — method, url, headers, body, mode, credentials, redirect, signal
- `FetchResponse` — status, statusText, headers, body, ok, redirected, url, type
- `AbortSignal` / `AbortController` — cancellation signal
- `CorsConfig` / `CorsResult` — origin matching, CORS check result
- `FetchInterceptor` / `FetchInterceptorChain` — request interception pipeline

### Functions

```
void  abort_controller_init(AbortController *ctrl);
void  abort_controller_abort(AbortController *ctrl);
int   abort_signal_aborted(const AbortSignal *signal);

void  cors_config_init(CorsConfig *cfg, const char *origin, ...);
CorsResult cors_check_simple(const FetchRequest *req, const CorsConfig *cfg);
CorsResult cors_check_preflight(const FetchRequest *req, ...);

void  fetch_interceptor_chain_init(FetchInterceptorChain *chain);
int   fetch_interceptor_chain_add(FetchInterceptorChain *chain, ...);
int   fetch_interceptor_chain_remove(FetchInterceptorChain *chain, ...);
FetchResponse *fetch_interceptor_chain_run(FetchInterceptorChain *chain, ...);

void  fetch_request_init(FetchRequest *req, const char *method, const char *url);
void  fetch_request_destroy(FetchRequest *req);
int   fetch_request_set_header(FetchRequest *req, const char *name, const char *value);
int   fetch_request_get_header(const FetchRequest *req, const char *name, ...);
void  fetch_request_set_body(FetchRequest *req, const char *body, size_t len);

void  fetch_response_init(FetchResponse *resp, int status, const char *statusText);
void  fetch_response_destroy(FetchResponse *resp);
int   fetch_response_set_header(FetchResponse *resp, const char *name, const char *value);
int   fetch_response_get_header(const FetchResponse *resp, const char *name, ...);
void  fetch_response_set_body(FetchResponse *resp, const char *body, size_t len);
FetchResponse *fetch_response_clone(const FetchResponse *resp);
```

---

## web_storage.h

### Types

- `WSKeyValue` — key, value ptr, value_len, created, modified timestamps
- `WSLocalStorage` — persistent per-origin storage with quota tracking, dirty flag, stats
- `WSSessionStorage` — per-tab session storage
- `StorageEventType` — `STORAGE_EVENT_SET`, `STORAGE_EVENT_REMOVE`, `STORAGE_EVENT_CLEAR`
- `StorageEvent` — event_type, origin, key, old_value, new_value, timestamp
- `StorageEventListener` — callback `void (*)(const StorageEvent*, void*)`
- `StorageHub` — manages localStorage + multiple sessionStorages, fires cross-tab events
- `IDBRecord` — key + value
- `IDBIndex` — name, sorted_indices array, unique flag
- `IDBObjectStore` — records array, key_path, auto_increment, indexes
- `IDBDatabase` — stores array, name, version
- `IDBCursorDirection` — `CURSOR_NEXT`, `CURSOR_NEXT_UNIQUE`, `CURSOR_PREV`, `CURSOR_PREV_UNIQUE`
- `IDBCursor` — cursor position, associated store/index, value access

### Functions

```
/* LocalStorage */
void ws_localstorage_init(WSLocalStorage *ls, const char *origin, size_t quota);
void ws_localstorage_destroy(WSLocalStorage *ls);
int  ws_localstorage_set(WSLocalStorage *ls, const char *key, const char *value);
int  ws_localstorage_get(const WSLocalStorage *ls, const char *key, ...);
int  ws_localstorage_remove(WSLocalStorage *ls, const char *key);
int  ws_localstorage_has(const WSLocalStorage *ls, const char *key);
void ws_localstorage_clear(WSLocalStorage *ls);
int  ws_localstorage_count(const WSLocalStorage *ls);
int  ws_localstorage_key(const WSLocalStorage *ls, int index, ...);
size_t ws_localstorage_remaining(const WSLocalStorage *ls);
int  ws_localstorage_save(const WSLocalStorage *ls, const char *filepath);
int  ws_localstorage_load(WSLocalStorage *ls, const char *filepath);

/* SessionStorage */
void ws_sessionstorage_init(WSSessionStorage *ss, const char *tab_id, ...);
void ws_sessionstorage_destroy(WSSessionStorage *ss);
int  ws_sessionstorage_set(WSSessionStorage *ss, const char *key, const char *value);
int  ws_sessionstorage_get(const WSSessionStorage *ss, const char *key, ...);
int  ws_sessionstorage_remove(WSSessionStorage *ss, const char *key);
int  ws_sessionstorage_has(const WSSessionStorage *ss, const char *key);
void ws_sessionstorage_clear(WSSessionStorage *ss);
int  ws_sessionstorage_count(const WSSessionStorage *ss);

/* StorageHub */
void storage_hub_init(StorageHub *hub, const char *origin, ...);
void storage_hub_destroy(StorageHub *hub);
int  storage_hub_open_tab(StorageHub *hub, const char *tab_id);
int  storage_hub_close_tab(StorageHub *hub, const char *tab_id);
int  storage_hub_local_set(StorageHub *hub, const char *key, const char *value);
int  storage_hub_local_get(StorageHub *hub, const char *key, ...);
int  storage_hub_local_remove(StorageHub *hub, const char *key);
void storage_hub_local_clear(StorageHub *hub);
int  storage_hub_session_set(StorageHub *hub, const char *tab_id, ...);
int  storage_hub_session_get(StorageHub *hub, const char *tab_id, ...);
void storage_hub_on_storage(StorageHub *hub, StorageEventListener, void*);
void storage_hub_fire_event(StorageHub *hub, const StorageEvent *event);

/* IndexedDB */
void idb_database_init(IDBDatabase *db, const char *name, int version);
void idb_database_destroy(IDBDatabase *db);
int  idb_create_store(IDBDatabase *db, const char *name, ...);
IDBObjectStore *idb_get_store(IDBDatabase *db, const char *name);
int  idb_delete_store(IDBDatabase *db, const char *name);
int  idb_store_put(IDBObjectStore *store, const char *key, ...);
int  idb_store_get(const IDBObjectStore *store, const char *key, ...);
int  idb_store_delete(IDBObjectStore *store, const char *key);
int  idb_store_count(const IDBObjectStore *store);
void idb_store_clear(IDBObjectStore *store);
int  idb_create_index(IDBObjectStore *store, const char *name, ...);
void idb_store_rebuild_indexes(IDBObjectStore *store);
void idb_cursor_open(IDBCursor *cursor, IDBDatabase *db, ...);
int  idb_cursor_next(IDBCursor *cursor);
int  idb_cursor_valid(const IDBCursor *cursor);
void idb_cursor_value(const IDBCursor *cursor, char *out, size_t *len);
void idb_cursor_key(const IDBCursor *cursor, char *out, size_t max);
```
