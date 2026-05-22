#include "http_cache.h"
#include "cookie_storage.h"
#include "event_system.h"
#include "fetch_model.h"
#include "web_storage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* ================================================================
 *  demo_browser_shell — 模拟浏览器核心流程
 *
 *  1. 启动时恢复 localStorage
 *  2. 用户导航到 URL
 *  3. 检查 HTTP 缓存 (ETag / 304)
 *  4. 发起 Fetch 请求 (CORS 检查 + 拦截器)
 *  5. 解析 Set-Cookie 存入 CookieJar
 *  6. 触发 DOM 事件 (load, click, scroll)
 *  7. 用户交互修改 sessionStorage
 *  8. IndexedDB 读写
 *  9. 打印统计信息
 * ================================================================ */

static EventTarget document_el, body_el, btn_el;
static StorageHub storage;
static HttpCache http_cache;
static CookieJar cookies;
static FetchInterceptorChain interceptors;
static CorsConfig cors_cfg;
static IDBDatabase idb;
static int step = 0;

static void log_step(const char *msg) {
    printf("\n[Step %d] %s\n", ++step, msg);
    printf("========================\n");
}

static void on_load(const Event *ev, void *data) {
    printf("  [Event] LOAD fired on '%s'\n",
           ((EventTarget*)ev->current_target)->id);
    (void)data;
}

static void on_btn_click(const Event *ev, void *data) {
    printf("  [Event] CLICK: button clicked!\n");
    printf("  [Event] data: %.*s\n", ev->data_len, ev->data);
    (void)data;
}

static void on_storage_change(const StorageEvent *ev, void *data) {
    printf("  [StorageEvent] %s %s in %s (old='%s' new='%s')\n",
           ev->event_type == STORAGE_EVENT_SET   ? "SET" :
           ev->event_type == STORAGE_EVENT_REMOVE ? "REMOVE" : "CLEAR",
           ev->key, ev->origin,
           ev->old_value, ev->new_value);
    (void)data;
}

/* Interceptor: mock responses for certain URLs */
static FetchResponse *intercept_mock(const FetchRequest *req, void *data) {
    (void)data;
    if (strstr(req->url, "/api/user")) {
        FetchResponse *resp = (FetchResponse*)malloc(sizeof(FetchResponse));
        fetch_response_init(resp, 200, "OK");
        fetch_response_set_header(resp, "content-type", "application/json");
        fetch_response_set_header(resp, "set-cookie",
            "user_id=42; Path=/; HttpOnly; SameSite=Lax");
        fetch_response_set_header(resp, "cache-control",
            "max-age=300");
        fetch_response_set_header(resp, "etag", "\"user-v1\"");
        fetch_response_set_body(resp, "{\"name\":\"Alice\",\"age\":30}", 28);
        return resp;
    }
    if (strstr(req->url, "/api/config")) {
        FetchResponse *resp = (FetchResponse*)malloc(sizeof(FetchResponse));
        fetch_response_init(resp, 200, "OK");
        fetch_response_set_header(resp, "content-type", "application/json");
        fetch_response_set_header(resp, "cache-control",
            "max-age=5, stale-while-revalidate=10");
        fetch_response_set_header(resp, "last-modified",
            "Tue, 19 May 2026 10:00:00 GMT");
        fetch_response_set_body(resp, "{\"theme\":\"light\"}", 19);
        return resp;
    }
    return NULL;
}

static void simulate_fetch(const char *url) {
    FetchRequest req;
    FetchResponse *resp;
    CorsResult cors_r;
    CacheHeader cache_hdrs[32];
    int cache_hdr_count = 0;

    log_step("Simulating fetch...");

    fetch_request_init(&req, "GET", url);
    fetch_request_set_header(&req, "origin", "https://example.com");
    printf("  Request: %s %s\n", req.method, req.url);

    /* CORS check */
    cors_r = cors_check_simple(&req, &cors_cfg);
    printf("  CORS result: %s\n",
           cors_r == CORS_ALLOWED ? "ALLOWED" :
           cors_r == CORS_BLOCKED ? "BLOCKED" : "PREFLIGHT_REQUIRED");

    if (cors_r == CORS_BLOCKED) {
        printf("  [BLOCKED] Request blocked by CORS\n");
        goto cleanup;
    }

    /* Interceptor chain */
    resp = fetch_interceptor_chain_run(&interceptors, &req);
    if (!resp) {
        printf("  No interceptor matched — would make real HTTP call\n");
        goto cleanup;
    }

    printf("  Interceptor returned: %d %s\n", resp->status, resp->status_text);

    /* Cache the response */
    {
        int h;
        for (h = 0; h < resp->header_count; h++) {
            strncpy(cache_hdrs[h].name, resp->headers[h].name, 128);
            strncpy(cache_hdrs[h].value, resp->headers[h].value, 256);
        }
        cache_hdr_count = resp->header_count;
        cache_store(&http_cache, url, resp->body, resp->body_len,
                    cache_hdrs, cache_hdr_count);
    }

    /* Parse Set-Cookie */
    {
        int h;
        for (h = 0; h < resp->header_count; h++) {
            if (strcasecmp(resp->headers[h].name, "set-cookie") == 0) {
                Cookie ck;
                cookie_parse_set_cookie(&ck, resp->headers[h].value,
                    "example.com", "/");
                cookie_jar_insert(&cookies, &ck);
                printf("  Cookie stored: %s=%s\n", ck.name, ck.value);
            }
        }
    }

    fetch_response_destroy(resp);
    free(resp);
cleanup:
    fetch_request_destroy(&req);
}

int main(void) {
    printf("############################################\n");
    printf("#  mini-browser-base — Browser Shell Demo  #\n");
    printf("############################################\n");

    /* ----- Init all subsystems ----- */
    log_step("Initializing subsystems");

    cache_init(&http_cache, 10 * 1024 * 1024);  /* 10MB cache */
    cookie_jar_init(&cookies, "example.com");
    fetch_interceptor_chain_init(&interceptors);
    fetch_interceptor_chain_add(&interceptors, intercept_mock, NULL);
    cors_config_init(&cors_cfg, "https://example.com", "*", 0);
    storage_hub_init(&storage, "https://example.com",
                     5 * 1024 * 1024, 3 * 1024 * 1024);
    storage_hub_open_tab(&storage, "tab-main");
    storage_hub_on_storage(&storage, on_storage_change, NULL);
    idb_database_init(&idb, "my-browser-db", 1);
    idb_create_store(&idb, "bookmarks", "url", 0);
    idb_create_index(idb_get_store(&idb, "bookmarks"), "by_title", "title", 0);

    /* ----- Build DOM tree ----- */
    event_target_init(&document_el, "document");
    event_target_init(&body_el, "body");
    event_target_init(&btn_el, "#main-btn");
    event_target_append_child(&document_el, &body_el);
    event_target_append_child(&body_el, &btn_el);

    event_add_listener(&document_el, EVENT_TYPE_LOAD, on_load, NULL, 0, 0, 0);
    event_add_listener(&btn_el, EVENT_TYPE_CLICK, on_btn_click, NULL, 0, 0, 0);

    /* ----- Load saved localStorage ----- */
    log_step("Loading localStorage from disk");
    {
        int loaded = ws_localstorage_load(
            (WSLocalStorage*)&storage.localstorage,
            "demo_ls_data.txt");
        printf("  Loaded: %s\n", loaded ? "yes" : "no (first run)");
    }
    ws_localstorage_set(&storage.localstorage, "last-visit",
                        "2026-05-19T12:00:00Z");
    printf("  Set 'last-visit'\n");

    /* ----- Trigger LOAD event ----- */
    log_step("Dispatching LOAD event");
    event_dispatch_at(&document_el, EVENT_TYPE_LOAD, EVENT_FLAG_TRUSTED, NULL, 0);

    /* ----- Simulate page navigation ----- */
    log_step("Fetching /api/user");
    simulate_fetch("https://example.com/api/user");

    log_step("Fetching /api/config");
    simulate_fetch("https://example.com/api/config");

    /* ----- Demonstrate cache ----- */
    log_step("Cache hit test for /api/user");
    {
        char buf[65536];
        size_t blen = sizeof(buf);
        if (cache_lookup(&http_cache, "/api/user", buf, &blen, NULL, NULL)) {
            printf("  Cache HIT: %.*s\n", (int)blen, buf);
        } else {
            printf("  Cache MISS\n");
        }
        printf("  Hit ratio: %.2f%%\n", cache_hit_ratio(&http_cache) * 100.0);
    }

    /* ----- Simulate user click ----- */
    log_step("User clicks #main-btn");
    event_dispatch_at(&btn_el, EVENT_TYPE_CLICK,
                      EVENT_FLAG_BUBBLES,
                      "{\"pageX\":150,\"pageY\":300}", 27);

    /* ----- Simulate keydown ----- */
    log_step("User presses Enter key");
    event_add_listener(&body_el, EVENT_TYPE_KEYDOWN, on_load,
                       "body-keydown", 0, 0, 0);
    event_dispatch_at(&body_el, EVENT_TYPE_KEYDOWN,
                      EVENT_FLAG_BUBBLES, "Enter", 5);

    /* ----- sessionStorage interaction ----- */
    log_step("Storing tab state in sessionStorage");
    storage_hub_session_set(&storage, "tab-main", "scrollY", "420");
    {
        char sv[256];
        if (storage_hub_session_get(&storage, "tab-main", "scrollY", sv, 256))
            printf("  scrollY = %s\n", sv);
    }

    /* ----- Cross-tab simulation ----- */
    log_step("Simulating cross-tab localStorage change");
    {
        /* Tab-2 opens in the background */
        storage_hub_open_tab(&storage, "tab-background");
    }
    /* Tab-2 sets a value */
    storage_hub_local_set(&storage, "shared-key", "from-tab-2");
    /* Tab-2 removes a value */
    storage_hub_local_set(&storage, "temp-key", "to-be-removed");
    storage_hub_local_remove(&storage, "temp-key");
    /* Tab-2 clears all storage */
    storage_hub_local_clear(&storage);

    /* ----- IndexedDB operations ----- */
    log_step("IndexedDB: inserting bookmarks");
    {
        IDBObjectStore *store = idb_get_store(&idb, "bookmarks");
        idb_store_put(store, "bm1", "{\"url\":\"https://a.com\",\"title\":\"A\"}", 37);
        idb_store_put(store, "bm2", "{\"url\":\"https://b.com\",\"title\":\"B\"}", 37);
        idb_store_put(store, "bm3", "{\"url\":\"https://c.com\",\"title\":\"C\"}", 37);
        printf("  Store count: %d\n", idb_store_count(store));

        /* Cursor iterate */
        IDBCursor cursor;
        idb_cursor_open(&cursor, &idb, "bookmarks", NULL, CURSOR_NEXT);
        printf("  Iterating with cursor:\n");
        while (idb_cursor_valid(&cursor)) {
            char key[256], val[4096];
            size_t vlen = sizeof(val);
            idb_cursor_key(&cursor, key, sizeof(key));
            idb_cursor_value(&cursor, val, &vlen);
            printf("    [%s] %.*s\n", key, (int)vlen, val);
            idb_cursor_next(&cursor);
        }
    }

    /* ----- Persist localStorage to disk ----- */
    log_step("Saving localStorage to disk");
    ws_localstorage_save(&storage.localstorage, "demo_ls_data.txt");
    printf("  Saved %d keys\n", ws_localstorage_count(&storage.localstorage));

    /* ----- Final stats ----- */
    printf("\n############################################\n");
    printf("#  Final Statistics\n");
    printf("############################################\n");
    {
        uint64_t h, m, e, r;
        cache_get_stats(&http_cache, &h, &m, &e, &r);
        printf("Cache hits: %llu / misses: %llu / evictions: %llu / reval: %llu\n",
               (unsigned long long)h, (unsigned long long)m,
               (unsigned long long)e, (unsigned long long)r);
        printf("Hit ratio: %.2f%%\n", cache_hit_ratio(&http_cache) * 100.0);
        printf("Cookies in jar: %d\n", cookies.count);
        printf("LocalStorage keys: %d (used: %zu / quota: %zu)\n",
               ws_localstorage_count(&storage.localstorage),
               storage.localstorage.used, storage.localstorage.quota);
        printf("Active tabs: %d\n", storage.session_count);
        printf("IDB stores: %d\n", idb.store_count);
    }

    /* ----- Cleanup ----- */
    storage_hub_close_tab(&storage, "tab-main");
    storage_hub_close_tab(&storage, "tab-background");
    storage_hub_destroy(&storage);
    cache_destroy(&http_cache);
    cookie_jar_destroy(&cookies);
    idb_database_destroy(&idb);
    event_target_destroy(&btn_el);
    event_target_destroy(&body_el);
    event_target_destroy(&document_el);

    printf("\nBrowser shell demo complete.\n");
    return 0;
}
