# mini-browser-base — 浏览器基础 (C 语言实现)

纯 C99 实现的浏览器核心平台 API 模型。包含 HTTP 缓存、Cookie 管理、DOM 事件系统、Fetch API 模型和 Web Storage，适用于嵌入式浏览器、教学演示和协议理解。

## 模块

| 模块 | 文件 | 功能 |
|------|------|------|
| **HTTP Cache** | `http_cache.h/c` | LRU 缓存 + ETag/304 条件请求 + Cache-Control 指令 |
| **Cookie Storage** | `cookie_storage.h/c` | Set-Cookie 解析 + domain/path 匹配 + localStorage + sessionStorage |
| **Event System** | `event_system.h/c` | DOM 事件: 捕获→目标→冒泡 + addEventListener + 自定义事件 |
| **Fetch Model** | `fetch_model.h/c` | Fetch API: Request/Response + AbortController + CORS + 拦截器 |
| **Web Storage** | `web_storage.h/c` | localStorage/sessionStorage + StorageEvent + IndexedDB 模拟 |

## 编译

```sh
make all          # 编译所有目标和示例
make examples     # 编译 3 个示例程序
make demos        # 编译 2 个演示程序
make clean        # 清理
```

或手动:

```sh
# 示例
gcc -std=c99 -Wall http_cache.c example_http_cache.c -o example_http_cache
gcc -std=c99 -Wall cookie_storage.c example_cookie_storage.c -o example_cookie_storage
gcc -std=c99 -Wall event_system.c example_event_system.c -o example_event_system

# 演示
gcc -std=c99 -Wall http_cache.c cookie_storage.c event_system.c fetch_model.c web_storage.c demo_browser_shell.c -o demo_browser_shell
gcc -std=c99 -Wall http_cache.c cookie_storage.c event_system.c fetch_model.c web_storage.c demo_web_page.c -o demo_web_page
```

## 示例

### HTTP 缓存

```c
HttpCache cache;
cache_init(&cache, 1024 * 1024);

CacheHeader hdrs[] = {{"cache-control", "max-age=60"}, {"etag", "\"abc\""}};
cache_store(&cache, "/api/data", "{\"v\":1}", 7, hdrs, 2);

char buf[4096]; size_t len = sizeof(buf);
if (cache_lookup(&cache, "/api/data", buf, &len, NULL, NULL))
    printf("HIT: %s\n", buf);
```

### Cookie 解析

```c
CookieJar jar;
cookie_jar_init(&jar, "example.com");

Cookie cookie;
cookie_parse_set_cookie(&cookie,
    "session=abc; Domain=example.com; Path=/; HttpOnly; Secure; SameSite=Lax",
    "example.com", "/");
cookie_jar_insert(&jar, &cookie);
```

### 事件分发

```c
EventTarget doc, btn;
event_target_init(&doc, "document");
event_target_init(&btn, "button");
event_target_append_child(&doc, &btn);

event_add_listener(&btn, "click", my_handler, NULL, 0, 0, 0);
event_dispatch_at(&btn, "click", EVENT_FLAG_BUBBLES, NULL, 0);
```

### Fetch 请求

```c
FetchRequest req;
fetch_request_init(&req, "GET", "https://api.example.com/data");
fetch_request_set_header(&req, "Accept", "application/json");

FetchResponse *resp = fetch_interceptor_chain_run(&chain, &req);
if (resp) {
    printf("Status: %d, Body: %.*s\n", resp->status,
           (int)resp->body_len, resp->body);
    fetch_response_destroy(resp);
    free(resp);
}
```

### Web Storage

```c
StorageHub hub;
storage_hub_init(&hub, "https://example.com", 5*1024*1024, 3*1024*1024);
storage_hub_open_tab(&hub, "tab-1");

storage_hub_local_set(&hub, "theme", "dark");
storage_hub_session_set(&hub, "tab-1", "scroll", "100");

char val[256];
storage_hub_local_get(&hub, "theme", val, sizeof(val));  // => "dark"
```

## 目录结构

```
mini-browser-base/
├── README.md
├── Makefile
├── API_REFERENCE.md
├── DESIGN.md
├── http_cache.h
├── cookie_storage.h
├── event_system.h
├── fetch_model.h
├── web_storage.h
├── http_cache.c
├── cookie_storage.c
├── event_system.c
├── fetch_model.c
├── web_storage.c
├── example_http_cache.c
├── example_cookie_storage.c
├── example_event_system.c
├── demo_browser_shell.c
└── demo_web_page.c
```

## 许可证

MIT
