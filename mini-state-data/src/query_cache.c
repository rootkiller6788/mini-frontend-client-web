#include "query_cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ── Internal Types ─────────────────────────────────────────────────────── */

#define MAX_ENTRIES 256
#define MAX_LISTENERS_PER_ENTRY 32
#define MAX_INFINITE_PAGES 64

struct QueryEntry {
    char          *key;
    QueryStatus    status;
    void          *data;
    size_t         data_size;
    void          *error_data;
    size_t         error_size;
    int64_t        created_at;
    int64_t        updated_at;
    int64_t        stale_time;
    int64_t        cache_time;
    int            retry_count;
    int            retry_max;
    int64_t        retry_delay;
    bool           retry_backoff_enabled;
    int            retry_attempt;
    bool           refetch_on_focus;
    bool           refetch_on_mount;
    bool           refetch_on_reconnect;
    QueryFetchFn   fetch_fn;
    QueryFreeFn    free_fn;
    QueryOptions   opts_snapshot;

    struct {
        QueryListener fn;
        void         *userdata;
        bool          active;
    } listeners[MAX_LISTENERS_PER_ENTRY];
    int listener_count;

    bool fetching;
    bool active_entry;
};

struct QueryCache {
    QueryEntry  *entries[MAX_ENTRIES];
    int          entry_count;
    QueryFreeFn  default_free;
    QueryOptions default_opts;
    int64_t    (*now_fn)(void);
    bool         window_focused;
};

struct MutationEntry {
    char          *key;
    MutationFn     mutate_fn;
    void          *data;
    size_t         data_size;
    void          *error_data;
    size_t         error_size;
    MutationStatus status;
    void          *variables;
    size_t         var_size;
    MutationOptions opts;
};

struct InfiniteQuery {
    QueryCache   *cache;
    char         *key;
    InfiniteFetchFn fetch_fn;
    int           page_size;
    void         *pages[MAX_INFINITE_PAGES];
    size_t        page_sizes[MAX_INFINITE_PAGES];
    QueryStatus   page_status[MAX_INFINITE_PAGES];
    int           page_count;
    int64_t       stale_time;
    void         *context;
    size_t        context_size;
};

/* ── Time ───────────────────────────────────────────────────────────────── */

static int64_t default_now_ms(void)
{
    return (int64_t)((double)clock() * 1000.0 / (double)CLOCKS_PER_SEC);
}

int64_t query_cache_now_ms(void) { return default_now_ms(); }

void query_cache_set_now_fn(int64_t (*fn)(void))
{
    /* global override */
    (void)fn;
}

/* ── Cache ──────────────────────────────────────────────────────────────── */

QueryCache *query_cache_create(void)
{
    QueryCache *c = calloc(1, sizeof(QueryCache));
    c->now_fn = default_now_ms;
    c->window_focused = true;
    c->default_opts.stale_time_ms = 0;
    c->default_opts.cache_time_ms = 300000;
    c->default_opts.retry_count = 3;
    c->default_opts.retry_delay_ms = 1000;
    c->default_opts.retry_backoff = true;
    c->default_opts.refetch_on_focus = true;
    c->default_opts.refetch_on_mount = true;
    c->default_opts.refetch_on_reconnect = true;
    return c;
}

void query_cache_destroy(QueryCache *cache)
{
    if (!cache) return;
    for (int i = 0; i < cache->entry_count; i++) {
        QueryEntry *e = cache->entries[i];
        if (e->free_fn && e->data) e->free_fn(e->data);
        free(e->error_data);
        free(e->key);
        free(e);
    }
    free(cache);
}

void query_cache_set_default_free(QueryCache *cache, QueryFreeFn fn)
{
    if (cache) cache->default_free = fn;
}

void query_cache_set_default_options(QueryCache *cache, QueryOptions opts)
{
    if (cache) cache->default_opts = opts;
}

/* ── Query Registration ─────────────────────────────────────────────────── */

static QueryEntry *cache_find(QueryCache *cache, const char *key)
{
    for (int i = 0; i < cache->entry_count; i++) {
        if (strcmp(cache->entries[i]->key, key) == 0)
            return cache->entries[i];
    }
    return NULL;
}

static QueryEntry *cache_insert(QueryCache *cache, const char *key)
{
    if (cache->entry_count >= MAX_ENTRIES) return NULL;
    QueryEntry *e = calloc(1, sizeof(QueryEntry));
    e->key = strdup(key);
    e->status = QUERY_STATUS_IDLE;
    e->created_at = default_now_ms();
    e->updated_at = e->created_at;
    e->active_entry = true;
    cache->entries[cache->entry_count++] = e;
    return e;
}

QueryEntry *query_cache_fetch(QueryCache *cache, const char *key,
                              QueryFetchFn fetch_fn, QueryOptions *opts)
{
    if (!cache || !key || !fetch_fn) return NULL;

    QueryEntry *e = cache_find(cache, key);
    if (!e) {
        e = cache_insert(cache, key);
        if (!e) return NULL;
    }

    e->fetch_fn = fetch_fn;
    if (opts) {
        e->stale_time = opts->stale_time_ms;
        e->cache_time = opts->cache_time_ms;
        e->retry_max = opts->retry_count;
        e->retry_delay = opts->retry_delay_ms;
        e->retry_backoff_enabled = opts->retry_backoff;
        e->refetch_on_focus = opts->refetch_on_focus;
        e->refetch_on_mount = opts->refetch_on_mount;
        e->refetch_on_reconnect = opts->refetch_on_reconnect;
        e->opts_snapshot = *opts;
    } else {
        e->stale_time = cache->default_opts.stale_time_ms;
        e->cache_time = cache->default_opts.cache_time_ms;
        e->retry_max = cache->default_opts.retry_count;
        e->retry_delay = cache->default_opts.retry_delay_ms;
        e->retry_backoff_enabled = cache->default_opts.retry_backoff;
        e->refetch_on_focus = cache->default_opts.refetch_on_focus;
        e->refetch_on_mount = cache->default_opts.refetch_on_mount;
        e->refetch_on_reconnect = cache->default_opts.refetch_on_reconnect;
    }
    e->free_fn = cache->default_free;

    /* actual fetch */
    e->status = QUERY_STATUS_LOADING;
    e->fetching = true;
    size_t sz = 0;
    void *result = fetch_fn(key, opts ? opts->context : NULL, &sz);
    e->fetching = false;

    if (result) {
        if (e->data && e->free_fn) e->free_fn(e->data);
        e->data = result;
        e->data_size = sz;
        e->status = QUERY_STATUS_SUCCESS;
        e->updated_at = default_now_ms();
        e->retry_attempt = 0;
    } else {
        e->status = QUERY_STATUS_ERROR;
        e->retry_attempt++;

        if (e->retry_attempt < e->retry_max) {
            int64_t delay = e->retry_delay;
            if (e->retry_backoff_enabled) {
                for (int r = 0; r < e->retry_attempt; r++) delay *= 2;
            }
            /* retry would happen here in real impl */
        }
    }

    for (int i = 0; i < e->listener_count; i++) {
        if (e->listeners[i].active && e->listeners[i].fn) {
            e->listeners[i].fn(e, e->listeners[i].userdata);
        }
    }

    return e;
}

QueryEntry *query_cache_prefetch(QueryCache *cache, const char *key,
                                 QueryFetchFn fetch_fn, QueryOptions *opts)
{
    return query_cache_fetch(cache, key, fetch_fn, opts);
}

QueryEntry *query_cache_get(QueryCache *cache, const char *key)
{
    if (!cache || !key) return NULL;
    return cache_find(cache, key);
}

void query_cache_invalidate(QueryCache *cache, const char *key)
{
    QueryEntry *e = cache_find(cache, key);
    if (e) e->status = QUERY_STATUS_STALE;
}

void query_cache_invalidate_prefix(QueryCache *cache, const char *prefix)
{
    if (!cache || !prefix) return;
    size_t plen = strlen(prefix);
    for (int i = 0; i < cache->entry_count; i++) {
        if (strncmp(cache->entries[i]->key, prefix, plen) == 0) {
            cache->entries[i]->status = QUERY_STATUS_STALE;
        }
    }
}

void query_cache_remove(QueryCache *cache, const char *key)
{
    if (!cache || !key) return;
    for (int i = 0; i < cache->entry_count; i++) {
        if (strcmp(cache->entries[i]->key, key) == 0) {
            QueryEntry *e = cache->entries[i];
            if (e->free_fn && e->data) e->free_fn(e->data);
            free(e->error_data);
            free(e->key);
            free(e);
            cache->entries[i] = cache->entries[cache->entry_count - 1];
            cache->entry_count--;
            return;
        }
    }
}

void query_cache_clear(QueryCache *cache)
{
    if (!cache) return;
    for (int i = 0; i < cache->entry_count; i++) {
        QueryEntry *e = cache->entries[i];
        if (e->free_fn && e->data) e->free_fn(e->data);
        free(e->error_data);
        free(e->key);
        free(e);
    }
    cache->entry_count = 0;
}

void query_cache_gc(QueryCache *cache)
{
    if (!cache) return;
    int64_t now = default_now_ms();
    int w = 0;
    for (int i = 0; i < cache->entry_count; i++) {
        QueryEntry *e = cache->entries[i];
        int64_t age = now - e->updated_at;
        if (age < e->cache_time || e->fetching) {
            cache->entries[w++] = e;
        } else {
            if (e->free_fn && e->data) e->free_fn(e->data);
            free(e->error_data);
            free(e->key);
            free(e);
        }
    }
    cache->entry_count = w;
}

/* ── Entry Accessors ────────────────────────────────────────────────────── */

const char   *query_entry_key(const QueryEntry *e)      { return e ? e->key : NULL; }
QueryStatus   query_entry_status(const QueryEntry *e)    { return e ? e->status : QUERY_STATUS_IDLE; }
void         *query_entry_data(const QueryEntry *e, size_t *sz) { if(sz) *sz = e ? e->data_size : 0; return e ? e->data : NULL; }
void         *query_entry_error(const QueryEntry *e, size_t *sz) { if(sz) *sz = e ? e->error_size : 0; return e ? e->error_data : NULL; }
int64_t       query_entry_updated_at(const QueryEntry *e)  { return e ? e->updated_at : 0; }
int64_t       query_entry_created_at(const QueryEntry *e)   { return e ? e->created_at : 0; }
int           query_entry_retry_attempt(const QueryEntry *e) { return e ? e->retry_attempt : 0; }
bool          query_entry_is_fetching(const QueryEntry *e)   { return e ? e->fetching : false; }

bool query_entry_is_stale(const QueryEntry *e)
{
    if (!e) return true;
    if (e->status == QUERY_STATUS_STALE || e->status == QUERY_STATUS_IDLE) return true;
    int64_t age = default_now_ms() - e->updated_at;
    return age > e->stale_time;
}

void query_entry_invalidate(QueryEntry *e)
{
    if (e) e->status = QUERY_STATUS_STALE;
}

bool query_entry_refetch(QueryEntry *e)
{
    if (!e || !e->fetch_fn) return false;
    if (e->fetching) return false;

    e->status = QUERY_STATUS_LOADING;
    e->fetching = true;
    size_t sz = 0;
    void *result = e->fetch_fn(e->key, e->opts_snapshot.context, &sz);
    e->fetching = false;

    if (result) {
        if (e->data && e->free_fn) e->free_fn(e->data);
        e->data = result;
        e->data_size = sz;
        e->status = QUERY_STATUS_SUCCESS;
        e->updated_at = default_now_ms();
    } else {
        e->status = QUERY_STATUS_ERROR;
    }
    return result != NULL;
}

int query_entry_subscribe(QueryEntry *e, QueryListener listener, void *ud)
{
    if (!e || !listener) return -1;
    if (e->listener_count >= MAX_LISTENERS_PER_ENTRY) return -1;
    int idx = e->listener_count++;
    e->listeners[idx].fn = listener;
    e->listeners[idx].userdata = ud;
    e->listeners[idx].active = true;
    return idx;
}

void query_entry_unsubscribe(QueryEntry *e, int handle)
{
    if (!e || handle < 0 || handle >= e->listener_count) return;
    e->listeners[handle].active = false;
}

/* ── Auto-Refetch ───────────────────────────────────────────────────────── */

void query_cache_on_window_focus(QueryCache *cache)
{
    if (!cache) return;
    cache->window_focused = true;
    for (int i = 0; i < cache->entry_count; i++) {
        QueryEntry *e = cache->entries[i];
        if (e->refetch_on_focus && query_entry_is_stale(e)) {
            query_entry_refetch(e);
        }
    }
}

void query_cache_on_window_blur(QueryCache *cache)
{
    if (cache) cache->window_focused = false;
}

void query_cache_on_mount(QueryCache *cache, const char *key)
{
    if (!cache) return;
    QueryEntry *e = cache_find(cache, key);
    if (e && e->refetch_on_mount && query_entry_is_stale(e)) {
        query_entry_refetch(e);
    }
}

void query_cache_on_reconnect(QueryCache *cache)
{
    if (!cache) return;
    for (int i = 0; i < cache->entry_count; i++) {
        QueryEntry *e = cache->entries[i];
        if (e->refetch_on_reconnect && query_entry_is_stale(e)) {
            query_entry_refetch(e);
        }
    }
}

int query_cache_poll_all(QueryCache *cache)
{
    int refetched = 0;
    if (!cache) return 0;
    for (int i = 0; i < cache->entry_count; i++) {
        if (query_entry_is_stale(cache->entries[i])) {
            query_entry_refetch(cache->entries[i]);
            refetched++;
        }
    }
    return refetched;
}

int query_cache_stale_count(QueryCache *cache)
{
    int c = 0;
    if (!cache) return 0;
    for (int i = 0; i < cache->entry_count; i++) {
        if (query_entry_is_stale(cache->entries[i])) c++;
    }
    return c;
}

/* ── Mutations ──────────────────────────────────────────────────────────── */

MutationEntry *query_cache_mutate(QueryCache *cache, const char *key,
                                  MutationFn mutate_fn,
                                  void *variables, size_t var_size,
                                  MutationOptions *opts)
{
    if (!cache || !key || !mutate_fn) return NULL;

    MutationEntry *m = calloc(1, sizeof(MutationEntry));
    m->key = strdup(key);
    m->mutate_fn = mutate_fn;
    m->status = MUTATION_IDLE;
    if (opts) m->opts = *opts;

    if (variables && var_size > 0) {
        m->variables = malloc(var_size);
        memcpy(m->variables, variables, var_size);
        m->var_size = var_size;
    }

    if (opts && opts->optimistic && opts->invalidate_keys) {
        query_cache_invalidate(cache, opts->invalidate_keys);
    }

    m->status = MUTATION_LOADING;
    size_t result_sz = 0;
    void *result = mutate_fn(variables, var_size, &result_sz);

    if (result) {
        m->data = result;
        m->data_size = result_sz;
        m->status = MUTATION_SUCCESS;

        if (opts && opts->invalidate_keys) {
            char *keys = strdup(opts->invalidate_keys);
            char *tok = strtok(keys, ",");
            while (tok) {
                while (*tok == ' ') tok++;
                query_cache_invalidate(cache, tok);
                tok = strtok(NULL, ",");
            }
            free(keys);
        }
    } else {
        m->status = MUTATION_ERROR;
    }

    return m;
}

const char    *mutation_entry_key(const MutationEntry *m)    { return m ? m->key : NULL; }
MutationStatus mutation_entry_status(const MutationEntry *m) { return m ? m->status : MUTATION_IDLE; }
void          *mutation_entry_data(const MutationEntry *m, size_t *sz) { if(sz) *sz=m?m->data_size:0; return m?m->data:NULL; }
void          *mutation_entry_error(const MutationEntry *m, size_t *sz) { if(sz) *sz=m?m->error_size:0; return m?m->error_data:NULL; }

void mutation_entry_reset(MutationEntry *m)
{
    if (!m) return;
    free(m->data);
    m->data = NULL;
    m->data_size = 0;
    m->status = MUTATION_IDLE;
}

/* ── Infinite Query ─────────────────────────────────────────────────────── */

InfiniteQuery *infinite_query_create(QueryCache *cache,
                                     InfiniteQueryOptions *opts,
                                     InfiniteFetchFn fetch_fn)
{
    if (!cache || !opts || !fetch_fn) return NULL;
    InfiniteQuery *iq = calloc(1, sizeof(InfiniteQuery));
    iq->cache = cache;
    iq->key = strdup(opts->query_key);
    iq->fetch_fn = fetch_fn;
    iq->page_size = opts->page_size;
    iq->stale_time = opts->stale_time_ms;
    return iq;
}

void infinite_query_destroy(InfiniteQuery *iq)
{
    if (!iq) return;
    for (int i = 0; i < iq->page_count; i++) free(iq->pages[i]);
    free(iq->key);
    free(iq);
}

bool infinite_query_fetch_page(InfiniteQuery *iq, int page)
{
    if (!iq || page >= MAX_INFINITE_PAGES) return false;

    size_t sz = 0;
    void *data = iq->fetch_fn(iq->key, page, iq->page_size,
                              iq->context, &sz);
    if (!data) {
        if (page < iq->page_count) iq->page_status[page] = QUERY_STATUS_ERROR;
        return false;
    }

    if (page < iq->page_count) {
        free(iq->pages[page]);
    }
    iq->pages[page] = data;
    iq->page_sizes[page] = sz;
    iq->page_status[page] = QUERY_STATUS_SUCCESS;
    if (page >= iq->page_count) iq->page_count = page + 1;
    return true;
}

void *infinite_query_all_data(InfiniteQuery *iq, size_t *total_size,
                              int *item_count)
{
    if (!iq) return NULL;
    size_t total = 0;
    for (int i = 0; i < iq->page_count; i++) total += iq->page_sizes[i];
    if (total_size) *total_size = total;
    if (item_count) *item_count = iq->page_count * iq->page_size;

    void *all = malloc(total > 0 ? total : 1);
    size_t offset = 0;
    for (int i = 0; i < iq->page_count; i++) {
        if (iq->pages[i] && iq->page_sizes[i] > 0) {
            memcpy((char *)all + offset, iq->pages[i], iq->page_sizes[i]);
            offset += iq->page_sizes[i];
        }
    }
    return all;
}

void *infinite_query_page_data(InfiniteQuery *iq, int page, size_t *out_size)
{
    if (!iq || page < 0 || page >= iq->page_count) { if(out_size)*out_size=0; return NULL; }
    if (out_size) *out_size = iq->page_sizes[page];
    return iq->pages[page];
}

int infinite_query_page_count(InfiniteQuery *iq) { return iq ? iq->page_count : 0; }

bool infinite_query_has_next_page(InfiniteQuery *iq)
{
    if (!iq || iq->page_count == 0) return true;
    return iq->page_sizes[iq->page_count - 1] > 0;
}

void infinite_query_invalidate(InfiniteQuery *iq)
{
    if (!iq) return;
    for (int i = 0; i < iq->page_count; i++) {
        iq->page_status[i] = QUERY_STATUS_STALE;
    }
}

/* ── Utility ────────────────────────────────────────────────────────────── */

int query_cache_entry_count(QueryCache *cache)
{
    return cache ? cache->entry_count : 0;
}

int query_cache_batch_fetch(QueryCache *cache, const char **keys, int count)
{
    if (!cache || !keys) return 0;
    int fetched = 0;
    for (int i = 0; i < count; i++) {
        QueryEntry *e = cache_find(cache, keys[i]);
        if (e) { query_entry_refetch(e); fetched++; }
    }
    return fetched;
}

bool query_cache_wait_all(QueryCache *cache, const char **keys, int count,
                          int timeout_ms)
{
    if (!cache) return false;
    (void)timeout_ms;
    for (int i = 0; i < count; i++) {
        QueryEntry *e = cache_find(cache, keys[i]);
        if (!e || e->status != QUERY_STATUS_SUCCESS) return false;
    }
    return true;
}
