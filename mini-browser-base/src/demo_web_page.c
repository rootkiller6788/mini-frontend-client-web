#include "http_cache.h"
#include "cookie_storage.h"
#include "event_system.h"
#include "fetch_model.h"
#include "web_storage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ================================================================
 *  demo_web_page — 模拟一个完整的 Web 页面生命周期
 *
 *  模拟场景:
 *    - 用户访问 https://shop.example.com
 *    - 浏览器发起 HTML + API + 静态资源请求
 *    - 使用 HTTP 缓存的 ETag/304 协议
 *    - 服务器通过 Set-Cookie 设置会话
 *    - 页面 JS 读取/写入 localStorage & sessionStorage
 *    - 用户点击"添加到购物车"按钮触发事件冒泡
 *    - IndexedDB 存储订单草稿
 *    - CORS 跨域 API 调用验证
 *    - AbortController 取消请求
 *    - 持久化关闭
 * ================================================================ */

/* --- Mock interceptor for shop API --- */
static FetchResponse *shop_interceptor(const FetchRequest *req, void *data) {
    (void)data;

    if (strstr(req->url, "/index.html")) {
        FetchResponse *r = (FetchResponse*)malloc(sizeof(FetchResponse));
        fetch_response_init(r, 200, "OK");
        fetch_response_set_header(r, "content-type", "text/html");
        fetch_response_set_header(r, "set-cookie",
            "session=shop-sess-xyz; Path=/; HttpOnly; Secure; SameSite=Lax");
        fetch_response_set_header(r, "cache-control", "max-age=60");
        fetch_response_set_header(r, "etag", "\"html-v3\"");
        fetch_response_set_body(r,
            "<html><body><button id='add-cart'>Add to Cart</button></body></html>", 67);
        return r;
    }
    if (strstr(req->url, "/api/products")) {
        FetchResponse *r = (FetchResponse*)malloc(sizeof(FetchResponse));
        fetch_response_init(r, 200, "OK");
        fetch_response_set_header(r, "content-type", "application/json");
        fetch_response_set_header(r, "cache-control",
            "max-age=10, stale-while-revalidate=30");
        fetch_response_set_header(r, "etag", "\"products-list-42\"");
        fetch_response_set_body(r,
            "[{\"id\":1,\"name\":\"Widget\",\"price\":9.99}]", 43);
        return r;
    }
    if (strstr(req->url, "/api/cart/add")) {
        FetchResponse *r = (FetchResponse*)malloc(sizeof(FetchResponse));
        fetch_response_init(r, 201, "Created");
        fetch_response_set_header(r, "content-type", "application/json");
        fetch_response_set_header(r, "set-cookie",
            "cart_count=1; Path=/; Max-Age=86400");
        fetch_response_set_body(r, "{\"success\":true,\"cartSize\":1}", 31);
        return r;
    }
    if (strstr(req->url, "/cdn/logo.png")) {
        FetchResponse *r = (FetchResponse*)malloc(sizeof(FetchResponse));
        fetch_response_init(r, 200, "OK");
        fetch_response_set_header(r, "content-type", "image/png");
        fetch_response_set_header(r, "cache-control",
            "max-age=86400, immutable");
        fetch_response_set_header(r, "etag", "\"logo-abc\"");
        fetch_response_set_body(r, "BINARY_PNG_DATA_HERE", 19);
        return r;
    }
    if (strstr(req->url, "analytics.example.com/track")) {
        FetchResponse *r = (FetchResponse*)malloc(sizeof(FetchResponse));
        fetch_response_init(r, 200, "OK");
        fetch_response_set_body(r, "{\"tracked\":true}", 16);
        return r;
    }
    return NULL;
}

static void print_cache_status(HttpCache *cache, const char *url) {
    char buf[65536];
    size_t blen = sizeof(buf);
    if (cache_lookup(cache, url, buf, &blen, NULL, NULL))
        printf("    [CACHE HIT] %s => %.*s\n", url, (int)blen, buf);
    else {
        int needs = cache_needs_revalidation(cache, url);
        printf("    [CACHE MISS] %s (needs_reval=%d)\n", url, needs);
    }
}

/* Event listener: prints event info */
static void print_event(const Event *ev, void *data) {
    const char *label = (const char*)data;
    printf("  [Event] %s: type='%s' phase=%d target='%s'\n",
           label, ev->type, ev->phase,
           ev->target ? ((EventTarget*)ev->target)->id : "?");
}

int main(void) {
    HttpCache cache;
    CookieJar cookies;
    FetchInterceptorChain chain;
    CorsConfig cors_cfg;
    CorsConfig cors_analytics;
    StorageHub storage;
    IDBDatabase idb;
    AbortController abort_ctrl;
    EventTarget doc, body, add_cart_btn, cart_icon;

    printf("#########################################\n");
    printf("#  demo_web_page — Web Page Lifecycle   #\n");
    printf("#########################################\n");
    printf("Origin: https://shop.example.com\n\n");

    /* ----- Init ----- */
    cache_init(&cache, 20 * 1024 * 1024);
    cookie_jar_init(&cookies, "shop.example.com");
    fetch_interceptor_chain_init(&chain);
    fetch_interceptor_chain_add(&chain, shop_interceptor, NULL);
    cors_config_init(&cors_cfg, "https://shop.example.com", "*", 0);
    cors_config_init(&cors_analytics, "https://shop.example.com",
                     "https://shop.example.com", 1);
    storage_hub_init(&storage, "https://shop.example.com",
                     5 * 1024 * 1024, 3 * 1024 * 1024);
    storage_hub_open_tab(&storage, "tab-1");
    storage_hub_on_storage(&storage, NULL, NULL);
    idb_database_init(&idb, "shop-db", 1);
    idb_create_store(&idb, "cart-drafts", "id", 1);
    {
        IDBObjectStore *st = idb_get_store(&idb, "cart-drafts");
        idb_create_index(st, "by-status", "status", 0);
    }
    abort_controller_init(&abort_ctrl);

    event_target_init(&doc, "document");
    event_target_init(&body, "body");
    event_target_init(&add_cart_btn, "#add-cart");
    event_target_init(&cart_icon, "#cart-icon");
    event_target_append_child(&doc, &body);
    event_target_append_child(&body, &add_cart_btn);
    event_target_append_child(&body, &cart_icon);

    /* Register listeners with capture/bubble */
    event_add_listener(&doc, EVENT_TYPE_CLICK, print_event, "doc-bubble", 0, 0, 0);
    event_add_listener(&doc, EVENT_TYPE_CLICK, print_event, "doc-capture", 1, 0, 0);
    event_add_listener(&body, EVENT_TYPE_CLICK, print_event, "body-bubble", 0, 0, 0);
    event_add_listener(&add_cart_btn, EVENT_TYPE_CLICK, print_event, "btn-target", 0, 0, 0);

    printf("=== Phase 1: Page Load ===\n\n");

    /* Fetch index.html */
    {
        FetchRequest req;
        FetchResponse *resp;
        CacheHeader hdrs[32];
        printf("[1] GET /index.html\n");
        fetch_request_init(&req, "GET", "https://shop.example.com/index.html");
        resp = fetch_interceptor_chain_run(&chain, &req);
        if (resp) {
            printf("    Response: %d %s\n", resp->status, resp->status_text);
            {
                int h;
                for (h = 0; h < resp->header_count; h++) {
                    if (strcasecmp(resp->headers[h].name, "set-cookie") == 0) {
                        Cookie ck;
                        cookie_parse_set_cookie(&ck, resp->headers[h].value,
                            "shop.example.com", "/");
                        cookie_jar_insert(&cookies, &ck);
                        printf("    Cookie: %s=%s (HttpOnly=%d, Secure=%d)\n",
                               ck.name, ck.value, ck.http_only, ck.secure);
                    }
                    strncpy(hdrs[h].name, resp->headers[h].name, 128);
                    strncpy(hdrs[h].value, resp->headers[h].value, 256);
                }
                cache_store(&cache, req.url, resp->body, resp->body_len,
                            hdrs, resp->header_count);
            }
            fetch_response_destroy(resp);
            free(resp);
        }
        fetch_request_destroy(&req);
    }

    printf("\n=== Phase 2: API Calls (cached) ===\n\n");

    /* Fetch products (first time) */
    printf("[2] GET /api/products\n");
    {
        FetchRequest req;
        FetchResponse *resp;
        CacheHeader hdrs[32];
        fetch_request_init(&req, "GET", "https://shop.example.com/api/products");
        resp = fetch_interceptor_chain_run(&chain, &req);
        if (resp) {
            printf("    Response: %d %s\n", resp->status, resp->status_text);
            {
                int h;
                for (h = 0; h < resp->header_count; h++) {
                    strncpy(hdrs[h].name, resp->headers[h].name, 128);
                    strncpy(hdrs[h].value, resp->headers[h].value, 256);
                }
                cache_store(&cache, req.url, resp->body, resp->body_len,
                            hdrs, resp->header_count);
            }
            fetch_response_destroy(resp);
            free(resp);
        }
        fetch_request_destroy(&req);
    }

    /* Fetch logo (immutable, cached forever) */
    printf("[3] GET /cdn/logo.png\n");
    {
        FetchRequest req;
        FetchResponse *resp;
        CacheHeader hdrs[32];
        fetch_request_init(&req, "GET", "https://shop.example.com/cdn/logo.png");
        resp = fetch_interceptor_chain_run(&chain, &req);
        if (resp) {
            int h;
            for (h = 0; h < resp->header_count; h++) {
                strncpy(hdrs[h].name, resp->headers[h].name, 128);
                strncpy(hdrs[h].value, resp->headers[h].value, 256);
            }
            cache_store(&cache, req.url, resp->body, resp->body_len,
                        hdrs, resp->header_count);
            printf("    Cached (immutable)\n");
            fetch_response_destroy(resp);
            free(resp);
        }
        fetch_request_destroy(&req);
    }

    /* Second fetch — check cache */
    printf("\n[4] Cache check: GET /api/products (2nd time)\n");
    print_cache_status(&cache, "https://shop.example.com/api/products");
    printf("[5] Cache check: GET /index.html\n");
    print_cache_status(&cache, "https://shop.example.com/index.html");

    /* Demonstrate conditional request */
    printf("\n[6] Conditional GET /index.html (if cache expired)\n");
    {
        const char *etag, *lm;
        cache_prepare_conditional(&cache,
            "https://shop.example.com/index.html", &etag, &lm);
        printf("    If-None-Match: %s\n", etag ? etag : "(none)");
        printf("    If-Modified-Since: %s\n", lm ? lm : "(none)");
        /* Server responds 304 */
        if (1 /* simulate 304 */) {
            CacheHeader h304[1];
            strcpy(h304[0].name, "etag");
            strcpy(h304[0].value, "\"html-v4\"");
            cache_update_with_304(&cache,
                "https://shop.example.com/index.html", h304, 1);
            printf("    Updated via 304 — new ETag: \"html-v4\"\n");
        }
    }

    printf("\n=== Phase 3: User Interaction ===\n\n");

    /* Load event */
    printf("[7] Dispatching load event\n");
    event_dispatch_at(&doc, EVENT_TYPE_LOAD, EVENT_FLAG_TRUSTED, NULL, 0);

    /* Scroll */
    printf("[8] Dispatching scroll event on body\n");
    event_add_listener(&body, EVENT_TYPE_SCROLL, print_event, "body-scroll", 0, 0, 0);
    event_dispatch_at(&body, EVENT_TYPE_SCROLL, EVENT_FLAG_BUBBLES,
                      "{\"scrollY\":250}", 15);

    /* localStorage usage */
    printf("[9] localStorage: save user preferences\n");
    ws_localstorage_set(&storage.localstorage, "pref-theme", "dark");
    ws_localstorage_set(&storage.localstorage, "pref-lang", "en");
    {
        char v[256];
        ws_localstorage_get(&storage.localstorage, "pref-theme", v, sizeof(v));
        printf("    pref-theme = %s\n", v);
        ws_localstorage_get(&storage.localstorage, "pref-lang", v, sizeof(v));
        printf("    pref-lang = %s\n", v);
        printf("    Remaining quota: %zu bytes\n",
               ws_localstorage_remaining(&storage.localstorage));
    }

    /* sessionStorage */
    printf("[10] sessionStorage: tab-specific state\n");
    storage_hub_session_set(&storage, "tab-1", "view-scroll", "400");
    {
        char s[128];
        storage_hub_session_get(&storage, "tab-1", "view-scroll", s, sizeof(s));
        printf("    view-scroll = %s\n", s);
    }

    /* Click event with full capture/bubble */
    printf("\n[11] Click #add-cart (capture → target → bubble)\n");
    event_dispatch_at(&add_cart_btn, EVENT_TYPE_CLICK,
        EVENT_FLAG_BUBBLES,
        "{\"productId\":1,\"action\":\"add\"}", 31);

    /* Add to cart API call */
    printf("\n[12] POST /api/cart/add\n");
    {
        FetchRequest req;
        FetchResponse *resp;
        fetch_request_init(&req, "POST", "https://shop.example.com/api/cart/add");
        fetch_request_set_header(&req, "content-type", "application/json");
        fetch_request_set_body(&req, "{\"productId\":1}", 15);
        resp = fetch_interceptor_chain_run(&chain, &req);
        if (resp) {
            printf("    Response: %d %s — body: %.*s\n",
                   resp->status, resp->status_text,
                   (int)resp->body_len, resp->body ? resp->body : "");
            {
                int h;
                for (h = 0; h < resp->header_count; h++) {
                    if (strcasecmp(resp->headers[h].name, "set-cookie") == 0) {
                        Cookie ck;
                        cookie_parse_set_cookie(&ck, resp->headers[h].value,
                            "shop.example.com", "/");
                        cookie_jar_insert(&cookies, &ck);
                        printf("    Cookie: %s=%s\n", ck.name, ck.value);
                    }
                }
            }
            fetch_response_destroy(resp);
            free(resp);
        }
        fetch_request_destroy(&req);
    }

    /* IndexedDB: save cart draft */
    printf("\n[13] IndexedDB: save cart draft\n");
    {
        IDBObjectStore *store = idb_get_store(&idb, "cart-drafts");
        idb_store_put(store, NULL,  /* auto-increment */
            "{\"items\":[{\"id\":1,\"qty\":1}],\"status\":\"draft\"}", 51);
        printf("    Store count: %d\n", idb_store_count(store));
    }

    printf("\n=== Phase 4: CORS Cross-Origin ===\n\n");

    /* Same-origin request passes */
    printf("[14] CORS: same-origin request\n");
    {
        FetchRequest req;
        CorsResult cr;
        fetch_request_init(&req, "GET", "https://shop.example.com/api/products");
        cr = cors_check_simple(&req, &cors_cfg);
        printf("    Result: %s\n",
               cr == CORS_ALLOWED ? "ALLOWED" : "BLOCKED");
        fetch_request_destroy(&req);
    }

    /* Cross-origin to analytics */
    printf("[15] CORS: cross-origin to analytics.example.com\n");
    {
        FetchRequest req;
        CorsResult cr;
        fetch_request_init(&req, "GET", "https://analytics.example.com/track");
        fetch_request_set_header(&req, "origin", "https://shop.example.com");
        cr = cors_check_simple(&req, &cors_analytics);
        printf("    Result: %s\n",
               cr == CORS_ALLOWED ? "ALLOWED" : "BLOCKED");
        fetch_request_destroy(&req);
    }

    /* AbortController demonstration */
    printf("\n[16] AbortController: cancel a request\n");
    printf("    Before abort: aborted=%d\n", abort_signal_aborted(&abort_ctrl.signal));
    abort_controller_abort(&abort_ctrl);
    printf("    After abort:  aborted=%d\n", abort_signal_aborted(&abort_ctrl.signal));

    printf("\n=== Phase 5: Stats & Teardown ===\n\n");

    /* Cache stats */
    {
        uint64_t h, m, e, r;
        cache_get_stats(&cache, &h, &m, &e, &r);
        printf("HTTP Cache: hits=%llu misses=%llu evict=%llu reval=%llu\n",
               (unsigned long long)h, (unsigned long long)m,
               (unsigned long long)e, (unsigned long long)r);
        printf("Hit ratio: %.2f%%\n", cache_hit_ratio(&cache) * 100.0);
    }

    printf("Cookie jar: %d cookies\n", cookies.count);
    printf("LocalStorage: %d keys\n", ws_localstorage_count(&storage.localstorage));
    printf("Active tabs: %d\n", storage.session_count);
    printf("IDB stores: %d, records in cart-drafts: %d\n",
           idb.store_count,
           idb_get_store(&idb, "cart-drafts") ?
               idb_store_count(idb_get_store(&idb, "cart-drafts")) : 0);

    /* Save localStorage */
    ws_localstorage_save(&storage.localstorage, "demo_web_page_ls.txt");
    printf("\nlocalStorage saved to demo_web_page_ls.txt\n");

    /* Cleanup */
    storage_hub_close_tab(&storage, "tab-1");
    storage_hub_destroy(&storage);
    cache_destroy(&cache);
    cookie_jar_destroy(&cookies);
    idb_database_destroy(&idb);
    event_target_destroy(&cart_icon);
    event_target_destroy(&add_cart_btn);
    event_target_destroy(&body);
    event_target_destroy(&doc);

    printf("\nWeb page lifecycle demo complete.\n");
    return 0;
}
