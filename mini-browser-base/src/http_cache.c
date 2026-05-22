#include "http_cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int cache_find_entry(const HttpCache *cache, const char *key) {
    int i;
    for (i = 0; i < cache->count; ++i) {
        if (strcmp(cache->entries[i].key, key) == 0)
            return i;
    }
    return -1;
}

static int cache_find_lru(const HttpCache *cache) {
    int i, idx = 0;
    time_t oldest = cache->entries[0].last_access;
    for (i = 1; i < cache->count; ++i) {
        if (cache->entries[i].last_access < oldest) {
            oldest = cache->entries[i].last_access;
            idx = i;
        }
    }
    return idx;
}

static void cache_evict_entry(HttpCache *cache, int idx) {
    CacheEntry *e = &cache->entries[idx];
    cache->current_size -= e->value_len + sizeof(*e->value);
    free(e->value);
    e->value = NULL;
    e->valid = 0;
    cache->evictions++;

    if (idx < cache->count - 1) {
        memmove(&cache->entries[idx], &cache->entries[idx + 1],
                (size_t)(cache->count - idx - 1) * sizeof(CacheEntry));
    }
    cache->count--;
}

void cache_init(HttpCache *cache, size_t max_size) {
    memset(cache, 0, sizeof(*cache));
    cache->max_size = max_size;
}

void cache_destroy(HttpCache *cache) {
    int i;
    for (i = 0; i < cache->count; ++i) {
        free(cache->entries[i].value);
    }
    memset(cache, 0, sizeof(*cache));
}

static void parse_directives(CacheEntry *entry, const char *val) {
    const char *p = val;
    const char *start;
    char token[128];
    int tlen;

    while (*p) {
        while (*p == ' ' || *p == ',') p++;
        if (!*p) break;
        start = p;
        while (*p && *p != ' ' && *p != ',' && *p != '=') p++;
        tlen = (int)(p - start);
        if (tlen > 127) tlen = 127;
        memcpy(token, start, (size_t)tlen);
        token[tlen] = '\0';

        if (strcmp(token, "max-age") == 0 && *p == '=') {
            p++;
            entry->max_age_sec = (int64_t)strtoll(p, (char**)&p, 10);
            entry->directives |= CACHE_DIRECTIVE_MAX_AGE;
        } else if (strcmp(token, "no-cache") == 0) {
            entry->directives |= CACHE_DIRECTIVE_NO_CACHE;
        } else if (strcmp(token, "no-store") == 0) {
            entry->directives |= CACHE_DIRECTIVE_NO_STORE;
        } else if (strcmp(token, "stale-while-revalidate") == 0 && *p == '=') {
            p++;
            entry->stale_while_revalidate_sec = (int64_t)strtoll(p, (char**)&p, 10);
            entry->directives |= CACHE_DIRECTIVE_STALE_WHILE_REV;
        } else if (strcmp(token, "public") == 0) {
            entry->directives |= CACHE_DIRECTIVE_PUBLIC;
        } else if (strcmp(token, "private") == 0) {
            entry->directives |= CACHE_DIRECTIVE_PRIVATE;
        } else if (strcmp(token, "must-revalidate") == 0) {
            entry->directives |= CACHE_DIRECTIVE_MUST_REVALIDATE;
        } else if (strcmp(token, "immutable") == 0) {
            entry->directives |= CACHE_DIRECTIVE_IMMUTABLE;
        }
        while (*p == ' ' || *p == ',') p++;
    }
}

void cache_parse_cache_control(CacheEntry *entry, const char *header_value) {
    entry->directives = 0;
    entry->max_age_sec = 0;
    entry->stale_while_revalidate_sec = 0;
    if (header_value && *header_value)
        parse_directives(entry, header_value);
}

int cache_is_fresh(const CacheEntry *entry) {
    if (!entry->valid) return 0;
    if (entry->directives & CACHE_DIRECTIVE_NO_STORE) return 0;
    if (entry->directives & CACHE_DIRECTIVE_IMMUTABLE) return 1;
    if (entry->directives & CACHE_DIRECTIVE_NO_CACHE) return 0;
    if (entry->expires_at > 0 && time(NULL) > entry->expires_at) {
        if ((entry->directives & CACHE_DIRECTIVE_STALE_WHILE_REV) &&
            time(NULL) <= entry->expires_at + entry->stale_while_revalidate_sec) {
            return 1;
        }
        return 0;
    }
    return 1;
}

int cache_store(HttpCache *cache, const char *key,
                const char *value, size_t value_len,
                const CacheHeader *headers, int header_count) {
    int i, idx;
    size_t need;

    idx = cache_find_entry(cache, key);

    /* Check no-store */
    {
        CacheEntry tmp;
        int hdr;
        memset(&tmp, 0, sizeof(tmp));
        for (hdr = 0; hdr < header_count; hdr++) {
            if (strcasecmp(headers[hdr].name, "cache-control") == 0) {
                cache_parse_cache_control(&tmp, headers[hdr].value);
                break;
            }
        }
        if (tmp.directives & CACHE_DIRECTIVE_NO_STORE) {
            if (idx >= 0) cache_invalidate(cache, key);
            return 0;
        }
    }

    need = value_len + sizeof(CacheEntry);
    while (cache->current_size + need > cache->max_size && cache->count > 0) {
        int victim = cache_find_lru(cache);
        cache_evict_entry(cache, victim);
        if (victim == idx) idx = -1;
    }

    if (idx < 0) {
        if (cache->count >= CACHE_MAX_ENTRIES) {
            cache_evict_entry(cache, cache_find_lru(cache));
        }
        idx = cache->count++;
    } else {
        free(cache->entries[idx].value);
        cache->current_size -= cache->entries[idx].value_len;
    }

    {
        CacheEntry *e = &cache->entries[idx];
        memset(e, 0, sizeof(*e));
        strncpy(e->key, key, CACHE_MAX_KEY_LEN - 1);
        e->value = (char*)malloc(value_len + 1);
        if (!e->value) return -1;
        memcpy(e->value, value, value_len);
        e->value[value_len] = '\0';
        e->value_len = value_len;
        e->created_at = time(NULL);
        e->last_access = e->created_at;
        e->valid = 1;
        e->access_count = 0;
        e->stale = 0;

        for (i = 0; i < header_count && i < CACHE_MAX_HEADER_COUNT; i++) {
            strncpy(e->headers[i].name, headers[i].name, CACHE_MAX_HEADER_LEN - 1);
            strncpy(e->headers[i].value, headers[i].value, CACHE_MAX_HEADER_LEN - 1);
            if (strcasecmp(headers[i].name, "etag") == 0)
                strncpy(e->etag, headers[i].value, CACHE_MAX_HEADER_LEN - 1);
            if (strcasecmp(headers[i].name, "last-modified") == 0)
                strncpy(e->last_modified, headers[i].value, CACHE_MAX_HEADER_LEN - 1);
            if (strcasecmp(headers[i].name, "cache-control") == 0)
                cache_parse_cache_control(e, headers[i].value);
        }
        e->header_count = (i < CACHE_MAX_HEADER_COUNT) ? i : CACHE_MAX_HEADER_COUNT;

        if (e->max_age_sec > 0)
            e->expires_at = e->created_at + e->max_age_sec;
        else
            e->expires_at = 0;

        cache->current_size += need;
    }
    return 1;
}

int cache_lookup(HttpCache *cache, const char *key,
                 char *value_out, size_t *value_len_out,
                 CacheHeader *headers_out, int *header_count_out) {
    int idx = cache_find_entry(cache, key);
    cache->total_requests++;
    if (idx < 0) {
        cache->misses++;
        return 0;
    }

    {
        CacheEntry *e = &cache->entries[idx];
        if (!cache_is_fresh(e)) {
            cache->misses++;
            return 0;
        }
        e->last_access = time(NULL);
        e->access_count++;

        if (value_out && value_len_out) {
            size_t copy = (e->value_len < *value_len_out) ? e->value_len : *value_len_out;
            memcpy(value_out, e->value, copy);
            *value_len_out = e->value_len;
        }
        if (headers_out && header_count_out) {
            int n = (e->header_count < *header_count_out) ? e->header_count : *header_count_out;
            memcpy(headers_out, e->headers, (size_t)n * sizeof(CacheHeader));
            *header_count_out = n;
        }
        cache->hits++;
    }
    return 1;
}

int cache_needs_revalidation(const HttpCache *cache, const char *key) {
    int idx = cache_find_entry(cache, key);
    if (idx < 0) return 1;
    return !cache_is_fresh(&cache->entries[idx]);
}

void cache_prepare_conditional(HttpCache *cache, const char *key,
                                const char **if_none_match,
                                const char **if_modified_since) {
    int idx = cache_find_entry(cache, key);
    *if_none_match = NULL;
    *if_modified_since = NULL;
    if (idx < 0) return;
    {
        CacheEntry *e = &cache->entries[idx];
        if (e->etag[0]) *if_none_match = e->etag;
        if (e->last_modified[0]) *if_modified_since = e->last_modified;
    }
}

int cache_update_with_304(HttpCache *cache, const char *key,
                           const CacheHeader *headers, int header_count) {
    int idx = cache_find_entry(cache, key);
    if (idx < 0) return 0;
    {
        CacheEntry *e = &cache->entries[idx];
        int i;
        for (i = 0; i < header_count; i++) {
            if (strcasecmp(headers[i].name, "etag") == 0)
                strncpy(e->etag, headers[i].value, CACHE_MAX_HEADER_LEN - 1);
            if (strcasecmp(headers[i].name, "cache-control") == 0)
                cache_parse_cache_control(e, headers[i].value);
        }
        e->created_at = time(NULL);
        e->last_access = e->created_at;
        if (e->max_age_sec > 0)
            e->expires_at = e->created_at + e->max_age_sec;
        cache->revalidations++;
    }
    return 1;
}

int cache_invalidate(HttpCache *cache, const char *key) {
    int idx = cache_find_entry(cache, key);
    if (idx < 0) return 0;
    cache_evict_entry(cache, idx);
    return 1;
}

int cache_invalidate_prefix(HttpCache *cache, const char *prefix) {
    int i, removed = 0;
    size_t plen = strlen(prefix);
    for (i = cache->count - 1; i >= 0; i--) {
        if (strncmp(cache->entries[i].key, prefix, plen) == 0) {
            cache_evict_entry(cache, i);
            removed++;
        }
    }
    return removed;
}

void cache_clear(HttpCache *cache) {
    while (cache->count > 0)
        cache_evict_entry(cache, 0);
}

void cache_get_stats(const HttpCache *cache,
                      uint64_t *hits, uint64_t *misses,
                      uint64_t *evictions, uint64_t *revalidations) {
    if (hits)          *hits          = cache->hits;
    if (misses)        *misses        = cache->misses;
    if (evictions)     *evictions     = cache->evictions;
    if (revalidations) *revalidations = cache->revalidations;
}

double cache_hit_ratio(const HttpCache *cache) {
    uint64_t total = cache->hits + cache->misses;
    if (total == 0) return 0.0;
    return (double)cache->hits / (double)total;
}
