#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "query_cache.h"
#include "zustand_sim.h"

/* ── Simulation: Async data fetching with caching ───────────────────────── */

typedef struct {
    int    id;
    char   name[64];
    char   email[128];
    bool   active;
} User;

/* Simulated API responses */
static void *fetch_users(const char *key, void *context, size_t *out_size)
{
    (void)key;
    (void)context;

    printf("  [network] Fetching users...\n");

    User *users = calloc(3, sizeof(User));
    users[0] = (User){1, "Alice", "alice@example.com", true};
    users[1] = (User){2, "Bob", "bob@example.com", true};
    users[2] = (User){3, "Charlie", "charlie@example.com", false};

    *out_size = 3 * sizeof(User);
    return users;
}

static void *fetch_user_by_id(const char *key, void *context, size_t *out_size)
{
    (void)context;
    printf("  [network] Fetching user: %s...\n", key);

    int id = key ? atoi(key + 5) : 0;  /* skip "user:" prefix */
    if (id < 1 || id > 3) {
        *out_size = 0;
        return NULL;
    }

    User *user = malloc(sizeof(User));
    switch (id) {
    case 1: *user = (User){1, "Alice", "alice@example.com", true}; break;
    case 2: *user = (User){2, "Bob", "bob@example.com", true}; break;
    case 3: *user = (User){3, "Charlie", "charlie@example.com", false}; break;
    }
    *out_size = sizeof(User);
    return user;
}

static void *fetch_posts_page(const char *key, int page, int page_size,
                              void *context, size_t *out_size)
{
    (void)key; (void)context;
    printf("  [network] Fetching posts page %d (size=%d)...\n", page, page_size);

    if (page > 3) { *out_size = 0; return NULL; }

    char *data = malloc(128);
    snprintf(data, 128, "Page %d: post%d, post%d, post%d",
             page, page * page_size + 1, page * page_size + 2,
             page * page_size + 3);
    *out_size = strlen(data) + 1;
    return data;
}

/* ── Mutation simulation ────────────────────────────────────────────────── */

static void *create_user_mutation(void *variables, size_t var_size,
                                  size_t *out_size)
{
    (void)var_size;
    User *new_user = (User *)variables;
    printf("  [mutation] Creating user: %s...\n", new_user ? new_user->name : "?");

    User *result = malloc(sizeof(User));
    if (new_user) memcpy(result, new_user, sizeof(User));
    result->id = 100;
    *out_size = sizeof(User);
    return result;
}

/* ── Main ───────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== mini-state-data: Async Data & Caching Demo ===\n\n");

    /* 1. Query Cache basics */
    printf("--- Query Cache: Fetch & Cache ---\n");
    QueryCache *cache = query_cache_create();

    QueryOptions opts = {0};
    opts.query_key = "users:list";
    opts.stale_time_ms = 5000;
    opts.cache_time_ms = 60000;
    opts.retry_count = 3;
    opts.retry_delay_ms = 1000;
    opts.retry_backoff = true;
    opts.refetch_on_focus = true;
    opts.refetch_on_mount = true;
    opts.refetch_on_reconnect = false;

    /* First fetch (cold) */
    printf("Fetching users (cold):\n");
    QueryEntry *entry = query_cache_fetch(cache, "users:list",
                                          fetch_users, &opts);
    if (entry) {
        size_t sz;
        User *users = (User *)query_entry_data(entry, &sz);
        int count = (int)(sz / sizeof(User));
        printf("  Got %d users (status=%d)\n", count, query_entry_status(entry));

        for (int i = 0; i < count; i++) {
            printf("    User %d: %s <%s> %s\n",
                   users[i].id, users[i].name, users[i].email,
                   users[i].active ? "[active]" : "[inactive]");
        }
    }

    /* Cached get (no network) */
    printf("\nGetting users (cached):\n");
    QueryEntry *cached = query_cache_get(cache, "users:list");
    if (cached) {
        printf("  Cache hit! status=%d, stale=%s\n",
               query_entry_status(cached),
               query_entry_is_stale(cached) ? "yes" : "no");
    }

    /* 2. Individual query */
    printf("\n--- Individual User Query ---\n");
    QueryOptions user_opts = {0};
    user_opts.query_key = "user:1";
    user_opts.stale_time_ms = 10000;
    user_opts.cache_time_ms = 60000;
    user_opts.retry_count = 2;

    QueryEntry *u1 = query_cache_fetch(cache, "user:1", fetch_user_by_id, &user_opts);
    if (u1) {
        size_t sz;
        User *user = (User *)query_entry_data(u1, &sz);
        if (user) printf("  User: %s <%s>\n", user->name, user->email);
    }

    /* 3. Auto-refetch on window focus */
    printf("\n--- Auto-Refetch on Focus ---\n");
    query_cache_on_window_blur(cache);
    query_cache_on_window_focus(cache); /* triggers refetch if stale */
    printf("  Entries in cache: %d\n", query_cache_entry_count(cache));

    /* 4. Cache invalidation */
    printf("\n--- Cache Invalidation ---\n");
    query_cache_invalidate(cache, "users:list");
    QueryEntry *inv = query_cache_get(cache, "users:list");
    printf("  After invalidate: status=%d, stale=%s\n",
           inv ? query_entry_status(inv) : -1,
           inv && query_entry_is_stale(inv) ? "yes" : "no");

    /* Re-fetch after invalidation */
    printf("  Re-fetching...\n");
    query_entry_refetch(inv);
    printf("  Status after refetch: %d\n", inv ? query_entry_status(inv) : -1);

    /* 5. Mutations */
    printf("\n--- Mutation: Create User ---\n");
    MutationOptions mut_opts = {0};
    mut_opts.key = "create-user";
    mut_opts.invalidate_keys = "users:list";
    mut_opts.optimistic = false;

    User new_user = {0, "Diana", "diana@example.com", true};
    MutationEntry *mutation = query_cache_mutate(cache, "create-user",
                                                  create_user_mutation,
                                                  &new_user, sizeof(User),
                                                  &mut_opts);
    if (mutation) {
        printf("  Mutation status: %d\n", mutation_entry_status(mutation));
        size_t sz;
        User *result = (User *)mutation_entry_data(mutation, &sz);
        if (result && mutation_entry_status(mutation) == MUTATION_SUCCESS) {
            printf("  Created user: %s (id=%d)\n", result->name, result->id);
        }
    }

    /* 6. Infinite query (pagination) */
    printf("\n--- Infinite Query: Paginated Posts ---\n");
    InfiniteQueryOptions inf_opts = {0};
    inf_opts.query_key = "posts:feed";
    inf_opts.page_size = 3;
    inf_opts.stale_time_ms = 30000;
    inf_opts.cache_time_ms = 120000;

    InfiniteQuery *iq = infinite_query_create(cache, &inf_opts, fetch_posts_page);

    for (int page = 0; page < 4; page++) {
        bool ok = infinite_query_fetch_page(iq, page);
        printf("  Page %d: %s\n", page, ok ? "loaded" : "empty");
    }

    printf("\n  All pages data:\n");
    for (int p = 0; p < infinite_query_page_count(iq); p++) {
        size_t sz;
        char *data = infinite_query_page_data(iq, p, &sz);
        printf("    %s\n", data ? data : "(null)");
    }

    size_t total_sz;
    int total_items;
    void *all_data = infinite_query_all_data(iq, &total_sz, &total_items);
    printf("  Total: %d items, %zu bytes\n", total_items, total_sz);
    free(all_data);

    /* 7. Zustand with async */
    printf("\n--- Zustand: Async Actions ---\n");
    typedef struct { bool loading; int user_count; } AsyncState;
    AsyncState ast = {false, 0};
    ZustandStore *zustand = zustand_store_create(&ast, sizeof(AsyncState));

    ZustandAsync *async = zustand_async_create(zustand);
    zustand_async_dispatch(async, NULL, NULL);

    AsyncState *cur = (AsyncState *)zustand_async_get_state(async, NULL);
    if (cur) printf("  Loading: %s, users: %d\n",
                     cur->loading ? "yes" : "no", cur->user_count);

    /* 8. Mutable to Immutable with produce */
    printf("\n--- Produce (immer-style) ---\n");
    typedef struct { int x; int y; } Point;
    Point pt = {10, 20};

    size_t produced_sz;
    void *produced = immut_produce(&pt, sizeof(Point),
                                   NULL, NULL, &produced_sz);
    printf("  Original: (%d,%d)\n", pt.x, pt.y);
    free(produced);

    /* Cleanup */
    printf("\n--- Cleanup ---\n");
    infinite_query_destroy(iq);
    zustand_async_destroy(async);
    zustand_store_destroy(zustand);
    query_cache_destroy(cache);

    printf("Done.\n");
    return 0;
}
