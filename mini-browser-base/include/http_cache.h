#ifndef HTTP_CACHE_H
#define HTTP_CACHE_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#define CACHE_MAX_ENTRIES        256
#define CACHE_MAX_KEY_LEN        512
#define CACHE_MAX_VALUE_LEN      65536
#define CACHE_MAX_HEADER_COUNT   32
#define CACHE_MAX_HEADER_LEN     256
#define CACHE_STALE_GRACE_SEC    30

typedef enum {
    CACHE_DIRECTIVE_NONE             = 0,
    CACHE_DIRECTIVE_MAX_AGE          = 1 << 0,
    CACHE_DIRECTIVE_NO_CACHE         = 1 << 1,
    CACHE_DIRECTIVE_NO_STORE         = 1 << 2,
    CACHE_DIRECTIVE_STALE_WHILE_REV  = 1 << 3,
    CACHE_DIRECTIVE_PUBLIC           = 1 << 4,
    CACHE_DIRECTIVE_PRIVATE          = 1 << 5,
    CACHE_DIRECTIVE_MUST_REVALIDATE  = 1 << 6,
    CACHE_DIRECTIVE_IMMUTABLE        = 1 << 7
} CacheDirective;

typedef struct {
    char name[CACHE_MAX_HEADER_LEN];
    char value[CACHE_MAX_HEADER_LEN];
} CacheHeader;

typedef struct {
    char  key[CACHE_MAX_KEY_LEN];
    char *value;
    size_t value_len;

    char etag[CACHE_MAX_HEADER_LEN];
    char last_modified[CACHE_MAX_HEADER_LEN];

    CacheHeader headers[CACHE_MAX_HEADER_COUNT];
    int header_count;

    uint32_t directives;
    int64_t  max_age_sec;
    int64_t  stale_while_revalidate_sec;

    time_t created_at;
    time_t expires_at;
    time_t last_access;

    uint32_t access_count;
    int      stale;
    int      valid;
} CacheEntry;

typedef struct {
    CacheEntry entries[CACHE_MAX_ENTRIES];
    int count;

    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
    uint64_t revalidations;
    uint64_t total_requests;

    size_t current_size;
    size_t max_size;
} HttpCache;

void  cache_init(HttpCache *cache, size_t max_size);
void  cache_destroy(HttpCache *cache);

int   cache_store(HttpCache *cache, const char *key,
                  const char *value, size_t value_len,
                  const CacheHeader *headers, int header_count);

int   cache_lookup(HttpCache *cache, const char *key,
                   char *value_out, size_t *value_len_out,
                   CacheHeader *headers_out, int *header_count_out);

int   cache_needs_revalidation(const HttpCache *cache, const char *key);
void  cache_prepare_conditional(HttpCache *cache, const char *key,
                                const char **if_none_match,
                                const char **if_modified_since);

int   cache_update_with_304(HttpCache *cache, const char *key,
                            const CacheHeader *headers, int header_count);

int   cache_invalidate(HttpCache *cache, const char *key);
int   cache_invalidate_prefix(HttpCache *cache, const char *prefix);
void  cache_clear(HttpCache *cache);

void  cache_parse_cache_control(CacheEntry *entry, const char *header_value);
int   cache_is_fresh(const CacheEntry *entry);

void  cache_get_stats(const HttpCache *cache,
                      uint64_t *hits, uint64_t *misses,
                      uint64_t *evictions, uint64_t *revalidations);

double cache_hit_ratio(const HttpCache *cache);

#endif
