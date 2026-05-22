#ifndef QUERY_CACHE_H
#define QUERY_CACHE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Core Types ─────────────────────────────────────────────────────────── */

typedef struct QueryCache    QueryCache;
typedef struct QueryEntry    QueryEntry;
typedef struct MutationEntry MutationEntry;
typedef struct InfiniteQuery InfiniteQuery;

typedef enum {
    QUERY_STATUS_IDLE,
    QUERY_STATUS_LOADING,
    QUERY_STATUS_SUCCESS,
    QUERY_STATUS_ERROR,
    QUERY_STATUS_STALE
} QueryStatus;

/* ── Query Options ──────────────────────────────────────────────────────── */

typedef struct QueryOptions {
    const char   *query_key;
    int64_t       stale_time_ms;
    int64_t       cache_time_ms;
    int           retry_count;
    int64_t       retry_delay_ms;
    bool          retry_backoff;      /* exponential backoff */
    bool          refetch_on_focus;
    bool          refetch_on_mount;
    bool          refetch_on_reconnect;
    void         *context;
    size_t        context_size;
} QueryOptions;

typedef void *(*QueryFetchFn)(const char *key, void *context, size_t *out_size);
typedef void  (*QueryFreeFn)(void *data);

typedef void (*QueryListener)(const QueryEntry *entry, void *userdata);

/* ── Query Cache ────────────────────────────────────────────────────────── */

QueryCache *query_cache_create(void);
void        query_cache_destroy(QueryCache *cache);

void        query_cache_set_default_free(QueryCache *cache, QueryFreeFn fn);
void        query_cache_set_default_options(QueryCache *cache,
                                            QueryOptions opts);

/* ── Query Registration ─────────────────────────────────────────────────── */

QueryEntry *query_cache_fetch(QueryCache *cache, const char *key,
                              QueryFetchFn fetch_fn, QueryOptions *opts);

QueryEntry *query_cache_prefetch(QueryCache *cache, const char *key,
                                 QueryFetchFn fetch_fn, QueryOptions *opts);

QueryEntry *query_cache_get(QueryCache *cache, const char *key);

void        query_cache_invalidate(QueryCache *cache, const char *key);
void        query_cache_invalidate_prefix(QueryCache *cache,
                                          const char *key_prefix);
void        query_cache_remove(QueryCache *cache, const char *key);
void        query_cache_clear(QueryCache *cache);
void        query_cache_gc(QueryCache *cache);

/* ── Query Entry Access ─────────────────────────────────────────────────── */

const char   *query_entry_key(const QueryEntry *entry);
QueryStatus   query_entry_status(const QueryEntry *entry);
void         *query_entry_data(const QueryEntry *entry, size_t *out_size);
void         *query_entry_error(const QueryEntry *entry, size_t *out_size);
int64_t       query_entry_updated_at(const QueryEntry *entry);
int64_t       query_entry_created_at(const QueryEntry *entry);
int           query_entry_retry_attempt(const QueryEntry *entry);
bool          query_entry_is_stale(const QueryEntry *entry);
bool          query_entry_is_fetching(const QueryEntry *entry);

void          query_entry_invalidate(QueryEntry *entry);
bool          query_entry_refetch(QueryEntry *entry);

int  query_entry_subscribe(QueryEntry *entry, QueryListener listener,
                           void *userdata);
void query_entry_unsubscribe(QueryEntry *entry, int handle);

/* ── Auto-Refetch Triggers ──────────────────────────────────────────────── */

void query_cache_on_window_focus(QueryCache *cache);
void query_cache_on_window_blur(QueryCache *cache);
void query_cache_on_mount(QueryCache *cache, const char *key);
void query_cache_on_reconnect(QueryCache *cache);
int  query_cache_poll_all(QueryCache *cache);
int  query_cache_stale_count(QueryCache *cache);

/* ── Mutations ──────────────────────────────────────────────────────────── */

typedef struct MutationOptions {
    const char *key;               /* mutation key for tracking */
    const char *invalidate_keys;   /* comma-separated keys to invalidate */
    bool        optimistic;
    void       *optimistic_data;
    size_t      optimistic_size;
    int         retry_count;
} MutationOptions;

typedef void *(*MutationFn)(void *variables, size_t var_size,
                            size_t *out_size);

MutationEntry *query_cache_mutate(QueryCache *cache, const char *key,
                                  MutationFn mutate_fn,
                                  void *variables, size_t var_size,
                                  MutationOptions *opts);

typedef enum {
    MUTATION_IDLE,
    MUTATION_LOADING,
    MUTATION_SUCCESS,
    MUTATION_ERROR
} MutationStatus;

const char   *mutation_entry_key(const MutationEntry *entry);
MutationStatus mutation_entry_status(const MutationEntry *entry);
void         *mutation_entry_data(const MutationEntry *entry, size_t *out_sz);
void         *mutation_entry_error(const MutationEntry *entry, size_t *out_sz);
void          mutation_entry_reset(MutationEntry *entry);

/* ── Infinite Query ─────────────────────────────────────────────────────── */

typedef void *(*InfiniteFetchFn)(const char *key, int page, int page_size,
                                 void *context, size_t *out_size);

typedef struct InfiniteQueryOptions {
    const char *query_key;
    int         page_size;
    int64_t     stale_time_ms;
    int64_t     cache_time_ms;
    int         retry_count;
} InfiniteQueryOptions;

InfiniteQuery *infinite_query_create(QueryCache *cache,
                                     InfiniteQueryOptions *opts,
                                     InfiniteFetchFn fetch_fn);
void           infinite_query_destroy(InfiniteQuery *iq);

bool           infinite_query_fetch_page(InfiniteQuery *iq, int page);
void          *infinite_query_all_data(InfiniteQuery *iq, size_t *total_size,
                                       int *item_count);
void          *infinite_query_page_data(InfiniteQuery *iq, int page,
                                        size_t *out_size);
int            infinite_query_page_count(InfiniteQuery *iq);
bool           infinite_query_has_next_page(InfiniteQuery *iq);
void           infinite_query_invalidate(InfiniteQuery *iq);

/* ── Utility ────────────────────────────────────────────────────────────── */

int64_t        query_cache_now_ms(void);
void           query_cache_set_now_fn(int64_t (*fn)(void));
int            query_cache_entry_count(QueryCache *cache);

/* ── Batch Operations ───────────────────────────────────────────────────── */

typedef struct {
    const char **keys;
    int          count;
    int          timeout_ms;
} QueryBatch;

int  query_cache_batch_fetch(QueryCache *cache, const char **keys, int count);
bool query_cache_wait_all(QueryCache *cache, const char **keys, int count,
                          int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* QUERY_CACHE_H */
