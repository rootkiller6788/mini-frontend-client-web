#include "http_cache.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    HttpCache cache;
    CacheHeader hdrs[4];
    char value_out[65536];
    size_t vlen;
    CacheHeader headers_out[32];
    int hdr_count;
    uint64_t hits, misses, evictions, reval;

    cache_init(&cache, 1024 * 1024); /* 1MB */

    /* --- Store entries with various Cache-Control directives --- */

    /* 1. max-age=60 */
    memset(hdrs, 0, sizeof(hdrs));
    strcpy(hdrs[0].name, "cache-control");
    strcpy(hdrs[0].value, "max-age=60");
    strcpy(hdrs[1].name, "etag");
    strcpy(hdrs[1].value, "\"abc123\"");
    cache_store(&cache, "/api/data", "{\"v\":1}", 7, hdrs, 2);

    /* 2. no-cache */
    memset(hdrs, 0, sizeof(hdrs));
    strcpy(hdrs[0].name, "cache-control");
    strcpy(hdrs[0].value, "no-cache");
    cache_store(&cache, "/api/live", "{\"v\":2}", 7, hdrs, 1);

    /* 3. stale-while-revalidate */
    memset(hdrs, 0, sizeof(hdrs));
    strcpy(hdrs[0].name, "cache-control");
    strcpy(hdrs[0].value, "max-age=1, stale-while-revalidate=30");
    strcpy(hdrs[1].name, "last-modified");
    strcpy(hdrs[1].value, "Mon, 01 Jan 2024 00:00:00 GMT");
    cache_store(&cache, "/api/stale", "{\"v\":3}", 7, hdrs, 2);

    /* --- Lookup --- */
    vlen = sizeof(value_out);
    hdr_count = 32;
    if (cache_lookup(&cache, "/api/data", value_out, &vlen,
                     headers_out, &hdr_count)) {
        printf("[HIT] /api/data => %s (len=%zu)\n", value_out, vlen);
    } else {
        printf("[MISS] /api/data\n");
    }

    vlen = sizeof(value_out);
    if (cache_lookup(&cache, "/api/live", value_out, &vlen, NULL, NULL)) {
        printf("[HIT] /api/live => %s\n", value_out);
    } else {
        printf("[MISS] /api/live (no-cache always misses)\n");
    }

    /* --- Conditional revalidation --- */
    {
        const char *etag, *lm;
        cache_prepare_conditional(&cache, "/api/data", &etag, &lm);
        printf("If-None-Match: %s\n", etag ? etag : "(none)");
        printf("If-Modified-Since: %s\n", lm ? lm : "(none)");
    }

    /* --- 304 update --- */
    {
        CacheHeader h304[2];
        strcpy(h304[0].name, "etag");
        strcpy(h304[0].value, "\"xyz789\"");
        strcpy(h304[1].name, "cache-control");
        strcpy(h304[1].value, "max-age=3600");
        cache_update_with_304(&cache, "/api/data", h304, 2);
        printf("Updated ETag after 304: ");
        {
            const char *e, *l;
            cache_prepare_conditional(&cache, "/api/data", &e, &l);
            printf("%s\n", e ? e : "(none)");
        }
    }

    /* --- Invalidation --- */
    printf("Invalidating /api/stale prefix...\n");
    cache_invalidate_prefix(&cache, "/api/stale");

    /* --- Stats --- */
    cache_get_stats(&cache, &hits, &misses, &evictions, &reval);
    printf("--- Cache Stats ---\n");
    printf("Hits:          %llu\n", (unsigned long long)hits);
    printf("Misses:        %llu\n", (unsigned long long)misses);
    printf("Evictions:     %llu\n", (unsigned long long)evictions);
    printf("Revalidations: %llu\n", (unsigned long long)reval);
    printf("Hit ratio:     %.2f%%\n", cache_hit_ratio(&cache) * 100.0);

    cache_destroy(&cache);
    return 0;
}
