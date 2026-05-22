# mini-browser-base — Design Document

## Overview

`mini-browser-base` implements the core browser platform APIs in portable C99. It simulates the foundational subsystems that every web browser relies on:

1. **HTTP Caching** — RFC 7234-compatible cache with conditional revalidation
2. **Cookie Storage** — RFC 6265 cookie handling + Web Storage
3. **DOM Events** — W3C DOM Level 3 Events (capture, target, bubble)
4. **Fetch API** — WHATWG Fetch spec model with CORS and interception
5. **Web Storage** — localStorage, sessionStorage, Storage events, IndexedDB concepts

## Architecture

```
┌────────────────────────────────────────────────────┐
│                   User Code                         │
├────────────────────────────────────────────────────┤
│  demo_browser_shell.c  │  demo_web_page.c            │
├────────────────────────────────────────────────────┤
│  example_http_cache.c  │  example_cookie_storage.c   │
│  example_event_system.c                              │
├────────────────────────────────────────────────────┤
│                 mini-browser-base                    │
│  ┌───────────┐  ┌──────────────┐  ┌─────────────┐  │
│  │http_cache │  │cookie_storage│  │event_system  │  │
│  │  .h/.c    │  │    .h/.c     │  │   .h/.c      │  │
│  └───────────┘  └──────────────┘  └─────────────┘  │
│  ┌───────────┐  ┌──────────────┐                    │
│  │fetch_model│  │ web_storage   │                    │
│  │  .h/.c    │  │    .h/.c      │                    │
│  └───────────┘  └──────────────┘                    │
└────────────────────────────────────────────────────┘
```

## Module Design

### 1. HTTP Cache (`http_cache.h/c`)

**Purpose**: Reduce network requests by caching HTTP responses.

**Implementation**:
- Fixed-size array of `CacheEntry` with LRU eviction (tracked by `last_access`)
- Memory quota tracking (`current_size` / `max_size`)
- Cache-Control parser supporting: `max-age`, `no-cache`, `no-store`, `stale-while-revalidate`, `public`, `private`, `must-revalidate`, `immutable`
- Conditional revalidation: stores ETag and Last-Modified, prepares `If-None-Match` / `If-Modified-Since` headers
- 304 response handling via `cache_update_with_304()`
- Prefix invalidation for cache-busting
- Comprehensive statistics (hits, misses, evictions, revalidations)

**Freshness Logic**:
- `immutable` → always fresh
- `no-store` → never cache
- `no-cache` → always requires revalidation
- `max-age` expired + `stale-while-revalidate` within grace period → serve stale
- Otherwise → check `expires_at`

### 2. Cookie Storage (`cookie_storage.h/c`)

**Purpose**: HTTP state management and client-side storage.

**Cookie Jar**:
- `cookie_parse_set_cookie()` parses RFC 6265 Set-Cookie headers
- Supports attributes: `Domain`, `Path`, `Expires`, `Max-Age`, `HttpOnly`, `Secure`, `SameSite`
- `cookie_domain_match()` implements domain matching (exact + suffix with leading dot)
- `cookie_path_match()` implements path matching (prefix)
- `cookie_jar_serialize_for_request()` builds Cookie header for outgoing requests
- Automatic expired cookie purging

**LocalStorage**:
- Per-origin persistent key-value store
- Quota enforcement
- Key enumeration (`localStorage.key(n)`)
- Same interface as Web API

**SessionStorage**:
- Per-tab key-value store (survives page reloads within same tab)
- Same API surface as localStorage
- Cleared on tab close

### 3. Event System (`event_system.h/c`)

**Purpose**: DOM event dispatching with full propagation model.

**Event Model**:
- `Event` struct carries type, target, currentTarget, phase, flags, timestamp, data
- Three propagation phases: `capture → at-target → bubble`
- `event_stop_propagation()` — stops further propagation
- `event_stop_immediate_propagation()` — also prevents remaining listeners on current target
- `event_prevent_default()` — signals that default action should not be taken

**EventTarget**:
- Tree structure with parent/children (simulates DOM hierarchy)
- `event_add_listener(type, callback, useCapture, once, passive)`
- `event_remove_listener()` — match by callback pointer identity
- `event_dispatch()` — builds propagation path from target to root, then dispatches in three phases
- Supports custom event types via arbitrary type strings
- `event_set_data()` for passing arbitrary payload

**Event Flags**:
- `EVENT_FLAG_BUBBLES` — event propagates up the tree
- `EVENT_FLAG_CANCELABLE` — can be cancelled
- `EVENT_FLAG_COMPOSED` — crosses shadow DOM boundaries
- `EVENT_FLAG_TRUSTED` — generated by user agent

### 4. Fetch Model (`fetch_model.h/c`)

**Purpose**: HTTP request/response modeling and interception.

**Request**:
- Method, URL, headers, body
- Mode (same-origin, cors, no-cors, navigate)
- Credentials mode
- Redirect mode
- AbortSignal reference

**Response**:
- Status code, status text
- Headers, body
- `ok` flag (2xx range)
- Response type classification

**AbortController**:
- Simple signal-based cancellation
- `abort_controller_abort()` sets flag; `abort_signal_aborted()` checks

**CORS**:
- `cors_check_simple()` — checks Origin against allowed origins for simple requests
- `cors_check_preflight()` — validates preflight requirements
- Origin matching: exact or wildcard `*`

**Request Interception**:
- `FetchInterceptorChain` — ordered list of interceptors
- Each interceptor can return a `FetchResponse*` (short-circuit) or NULL (pass through)
- Enables mocking, service worker simulation, and request rewriting

### 5. Web Storage (`web_storage.h/c`)

**Purpose**: Full client-side storage APIs.

**LocalStorage**:
- Dynamic value allocation (malloc)
- Quota tracking with remaining-space query
- Dirty flag for persistence optimization
- Save/Load to file for persistence across sessions
- Stats: total get/set counts

**SessionStorage**:
- Same structure as localStorage but scoped to a tab
- Tab identified by string ID

**StorageHub**:
- Owns one localStorage + N sessionStorages (one per tab)
- `storage_hub_open_tab()` / `storage_hub_close_tab()` manage lifecycle
- `storage_hub_local_set/remove/clear` fires StorageEvent
- `storage_hub_on_storage()` registers cross-tab listener
- Enables the browser's cross-tab localStorage synchronization

**IndexedDB Simulation**:
- `IDBDatabase` contains multiple `IDBObjectStore`s
- Each store has records (key-value), auto-increment keys, indexes
- `IDBCursor` iterates forward/backward over store or index
- `idb_store_rebuild_indexes()` updates index sorted arrays after mutations
- Key concepts: object store, index, cursor direction (next/prev/unique)

## Memory Management

All modules follow these rules:
- Init functions zero-initialize structs
- Destroy functions free all dynamic allocations
- No global state — all state passed via struct pointers
- Fixed-size arrays with defined max constants (auto-stack)
- Dynamic values use malloc/free (strings, bodies)

## Limitations (by design)

- Cookie parsing does not handle quoted-string values or cookie prefixes (`__Secure-`, `__Host-`)
- Event delegation is simulated; no real DOM integration
- CORS check is simplified — no header allow-lists in preflight
- IndexedDB indexes are rebuilt sequentially (O(n^2) worst case)
- No multi-threading — single-threaded event loop model
- All strings are null-terminated C strings, not counted buffers
