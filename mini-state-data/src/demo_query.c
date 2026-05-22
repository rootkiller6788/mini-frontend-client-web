#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "query_cache.h"
#include "immutable_store.h"
#include "zustand_sim.h"

/* ══════════════════════════════════════════════════════════════════════════
   Demo: Query Cache Full Pipeline — Fetch, Cache, Mutate, Infinite, Prefetch
   ══════════════════════════════════════════════════════════════════════════ */

/* ── Data Types ─────────────────────────────────────────────────────────── */

typedef struct {
    int    id;
    char   title[128];
    char   body[512];
    int    author_id;
    int    likes;
    bool   published;
} Article;

typedef struct {
    int    id;
    char   name[64];
    char   avatar[32];
} Author;

typedef struct {
    int    article_id;
    char   text[256];
    int    likes;
} Comment;

/* ── Simulated Network ──────────────────────────────────────────────────── */

static int network_call_count = 0;

static void *fetch_articles(const char *key, void *context, size_t *out_size)
{
    (void)key; (void)context;
    network_call_count++;
    printf("  [net#%d] GET /api/articles\n", network_call_count);

    Article *articles = calloc(4, sizeof(Article));
    articles[0] = (Article){1, "Introduction to C99", "C99 is a standard...", 1, 42, true};
    articles[1] = (Article){2, "State Management Patterns", "Redux, MobX...", 1, 128, true};
    articles[2] = (Article){3, "Building a Query Cache", "How to cache...", 2, 15, false};
    articles[3] = (Article){4, "Immutable Data Structures", "Tries and structural...", 2, 67, true};

    *out_size = 4 * sizeof(Article);
    return articles;
}

static void *fetch_author(const char *key, void *context, size_t *out_size)
{
    (void)context;
    network_call_count++;

    int id = 0;
    if (key) {
        const char *p = strrchr(key, ':');
        if (p) id = atoi(p + 1);
    }
    printf("  [net#%d] GET /api/authors/%d\n", network_call_count, id);

    Author *author = malloc(sizeof(Author));
    if (id == 1) {
        *author = (Author){1, "Alice Johnson", "alice.png"};
    } else if (id == 2) {
        *author = (Author){2, "Bob Smith", "bob.png"};
    } else {
        free(author);
        *out_size = 0;
        return NULL;
    }

    *out_size = sizeof(Author);
    return author;
}

static void *fetch_search(const char *key, void *context, size_t *out_size)
{
    (void)context;
    network_call_count++;
    const char *query = key ? key + 7 : ""; /* skip "search:" */
    printf("  [net#%d] GET /api/search?q=%s\n", network_call_count, query);

    char *result = malloc(256);
    snprintf(result, 256, "Search results for '%s': 3 items found", query);
    *out_size = strlen(result) + 1;
    return result;
}

static void *fetch_comments_page(const char *key, int page, int page_size,
                                  void *context, size_t *out_size)
{
    (void)key; (void)context;
    network_call_count++;
    printf("  [net#%d] GET /api/comments?page=%d&size=%d\n",
           network_call_count, page, page_size);

    Comment *comments = calloc(page_size, sizeof(Comment));
    for (int i = 0; i < page_size; i++) {
        int id = page * page_size + i;
        comments[i] = (Comment){id % 5 + 1, "", id % 3};
        snprintf(comments[i].text, sizeof(comments[i].text),
                 "Comment #%d on page %d", id, page);
    }

    *out_size = page_size * sizeof(Comment);
    return comments;
}

static void *create_article_mutation(void *variables, size_t var_size,
                                      size_t *out_size)
{
    (void)var_size;
    network_call_count++;
    Article *art = (Article *)variables;
    printf("  [net#%d] POST /api/articles (title='%s')\n",
           network_call_count, art ? art->title : "?");

    Article *result = malloc(sizeof(Article));
    if (art) memcpy(result, art, sizeof(Article));
    result->id = 999;
    result->published = true;
    *out_size = sizeof(Article);
    return result;
}

static void *update_article_mutation(void *variables, size_t var_size,
                                      size_t *out_size)
{
    (void)var_size;
    network_call_count++;
    Article *art = (Article *)variables;
    printf("  [net#%d] PUT /api/articles/%d\n",
           network_call_count, art ? art->id : -1);
    *out_size = sizeof(Article);
    Article *result = malloc(sizeof(Article));
    if (art) memcpy(result, art, sizeof(Article));
    return result;
}

/* ── Helper: print article ──────────────────────────────────────────────── */

static void print_article(int index, const Article *a)
{
    printf("  [%d] #%d '%s' by author#%d, %d likes, %s\n",
           index, a->id, a->title, a->author_id, a->likes,
           a->published ? "published" : "draft");
}

/* ── Helper: print author ───────────────────────────────────────────────── */

static void print_author(const Author *a)
{
    printf("  Author: %s (avatar: %s)\n", a->name, a->avatar);
}

/* ── Main Demo ──────────────────────────────────────────────────────────── */

int main(void)
{
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║     mini-state-data: Query Cache Full Pipeline Demo         ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    network_call_count = 0;

    /* ── 1. Initialize ────────────────────────────────────────────────── */
    printf("── 1. Creating Query Cache ──────────────────────────────────\n");
    QueryCache *cache = query_cache_create();
    printf("    Cache created. Default stale=%lldms, cache=%lldms\n",
           (long long)cache->default_opts.stale_time_ms,
           (long long)cache->default_opts.cache_time_ms);

    /* ── 2. Basic Fetch ──────────────────────────────────────────────── */
    printf("\n── 2. Fetching Articles (cold) ──────────────────────────────\n");

    QueryOptions article_opts = {0};
    article_opts.query_key = "articles:list";
    article_opts.stale_time_ms = 10000;
    article_opts.cache_time_ms = 60000;
    article_opts.retry_count = 3;
    article_opts.retry_delay_ms = 1000;
    article_opts.retry_backoff = true;
    article_opts.refetch_on_focus = true;
    article_opts.refetch_on_mount = true;
    article_opts.refetch_on_reconnect = true;

    QueryEntry *articles_entry = query_cache_fetch(cache, "articles:list",
                                                    fetch_articles,
                                                    &article_opts);

    if (articles_entry) {
        size_t sz;
        Article *arts = (Article *)query_entry_data(articles_entry, &sz);
        int count = (int)(sz / sizeof(Article));

        printf("    Fetched %d articles (status=%d, stale=%s):\n",
               count, query_entry_status(articles_entry),
               query_entry_is_stale(articles_entry) ? "yes" : "no");

        for (int i = 0; i < count; i++) print_article(i, &arts[i]);
    }

    printf("    Network calls so far: %d\n", network_call_count);

    /* ── 3. Cache Hit (no network) ───────────────────────────────────── */
    printf("\n── 3. Cache Hit Test ───────────────────────────────────────\n");

    QueryEntry *cached = query_cache_get(cache, "articles:list");
    if (cached && !query_entry_is_stale(cached)) {
        size_t sz;
        Article *arts = (Article *)query_entry_data(cached, &sz);
        int count = (int)(sz / sizeof(Article));
        printf("    Cache HIT: %d articles (zero network calls)\n", count);
        printf("    First article: '%s'\n", arts[0].title);
    }
    printf("    Network calls: %d (no increase expected)\n", network_call_count);

    /* ── 4. Fetch Authors (parallel-like) ─────────────────────────────── */
    printf("\n── 4. Fetching Authors ──────────────────────────────────────\n");

    QueryOptions author_opts = {0};
    author_opts.query_key = "author:1";
    author_opts.stale_time_ms = 30000;
    author_opts.cache_time_ms = 120000;

    QueryEntry *a1 = query_cache_fetch(cache, "author:1", fetch_author,
                                        &author_opts);
    QueryEntry *a2 = query_cache_fetch(cache, "author:2", fetch_author,
                                        &author_opts);

    if (a1) {
        size_t sz;
        Author *author = (Author *)query_entry_data(a1, &sz);
        if (author && query_entry_status(a1) == QUERY_STATUS_SUCCESS)
            print_author(author);
    }
    if (a2) {
        size_t sz;
        Author *author = (Author *)query_entry_data(a2, &sz);
        if (author && query_entry_status(a2) == QUERY_STATUS_SUCCESS)
            print_author(author);
    }

    printf("    Network calls: %d\n", network_call_count);

    /* ── 5. Window Focus Refetch ─────────────────────────────────────── */
    printf("\n── 5. Window Focus Auto-Refetch ─────────────────────────────\n");

    printf("    Simulating window blur + focus...\n");
    query_cache_on_window_blur(cache);
    printf("    Network calls after blur: %d\n", network_call_count);

    query_cache_on_window_focus(cache);
    printf("    Network calls after focus (stale refetched): %d\n",
           network_call_count);

    /* ── 6. Manual Refetch ───────────────────────────────────────────── */
    printf("\n── 6. Manual Refetch ────────────────────────────────────────\n");

    printf("    Manually refetching articles...\n");
    bool ok = query_entry_refetch(articles_entry);
    printf("    Refetch: %s\n", ok ? "OK" : "FAILED");
    printf("    Network calls: %d\n", network_call_count);

    /* ── 7. Cache Invalidation ────────────────────────────────────────── */
    printf("\n── 7. Cache Invalidation ────────────────────────────────────\n");

    printf("    Before invalidate: stale=%s\n",
           query_entry_is_stale(articles_entry) ? "yes" : "no");

    query_cache_invalidate(cache, "articles:list");
    printf("    After invalidate: stale=%s\n",
           query_entry_is_stale(articles_entry) ? "yes" : "no");

    printf("    Invalidating by prefix 'author':\n");
    query_cache_invalidate_prefix(cache, "author");
    printf("    Author caches invalidated.\n");

    /* ── 8. Mutations ────────────────────────────────────────────────── */
    printf("\n── 8. Mutations ──────────────────────────────────────────────\n");

    /* Create article */
    MutationOptions create_opts = {0};
    create_opts.key = "create-article";
    create_opts.invalidate_keys = "articles:list";
    create_opts.optimistic = true;

    Article new_art = {0, "New Article from Mutation", "Content here...",
                       1, 0, false};
    MutationEntry *m = query_cache_mutate(cache, "create-article",
                                           create_article_mutation,
                                           &new_art, sizeof(Article),
                                           &create_opts);
    if (m) {
        printf("    Create mutation: status=%d\n", mutation_entry_status(m));
        size_t sz;
        Article *result = (Article *)mutation_entry_data(m, &sz);
        if (result && mutation_entry_status(m) == MUTATION_SUCCESS) {
            printf("    Created: #%d '%s'\n", result->id, result->title);
        }
        mutation_entry_reset(m);
        free(m);
    }

    /* Update article */
    MutationOptions update_opts = {0};
    update_opts.key = "update-article";
    update_opts.invalidate_keys = "articles:list,articles:2";

    Article update_art = {2, "Updated Title", "Updated body...", 2, 99, true};
    m = query_cache_mutate(cache, "update-article",
                           update_article_mutation,
                           &update_art, sizeof(Article),
                           &update_opts);
    if (m) {
        printf("    Update mutation: status=%d\n", mutation_entry_status(m));
        free(m);
    }

    printf("    Network calls: %d\n", network_call_count);

    /* ── 9. Infinite Query ───────────────────────────────────────────── */
    printf("\n── 9. Infinite Query (Pagination) ───────────────────────────\n");

    InfiniteQueryOptions inf_opts = {0};
    inf_opts.query_key = "comments:article-1";
    inf_opts.page_size = 3;
    inf_opts.stale_time_ms = 30000;
    inf_opts.cache_time_ms = 120000;

    InfiniteQuery *iq = infinite_query_create(cache, &inf_opts,
                                              fetch_comments_page);

    for (int page = 0; page < 5; page++) {
        bool has = infinite_query_fetch_page(iq, page);
        printf("    Page %d: %s, has_next=%s\n", page,
               has ? "loaded" : "empty",
               infinite_query_has_next_page(iq) ? "yes" : "no");
    }

    printf("    Total pages: %d\n", infinite_query_page_count(iq));

    /* Print page summaries */
    for (int p = 0; p < infinite_query_page_count(iq); p++) {
        size_t psz;
        Comment *page_data = infinite_query_page_data(iq, p, &psz);
        int items = (int)(psz / sizeof(Comment));
        printf("    Page %d: %d comments\n", p, items);
    }

    /* ── 10. Prefetch ────────────────────────────────────────────────── */
    printf("\n── 10. Prefetch ─────────────────────────────────────────────\n");

    QueryOptions prefetch_opts = {0};
    prefetch_opts.query_key = "search:prefetch";
    prefetch_opts.stale_time_ms = 60000;
    prefetch_opts.cache_time_ms = 300000;

    QueryEntry *prefetched = query_cache_prefetch(cache, "search:prefetch",
                                                   fetch_search,
                                                   &prefetch_opts);
    if (prefetched) {
        size_t sz;
        char *result = (char *)query_entry_data(prefetched, &sz);
        printf("    Prefetched: %s\n", result ? result : "(null)");
    }

    /* ── 11. Cache GC ────────────────────────────────────────────────── */
    printf("\n── 11. Cache Garbage Collection ─────────────────────────────\n");
    int before_gc = query_cache_entry_count(cache);
    printf("    Entries before GC: %d\n", before_gc);
    query_cache_gc(cache);
    int after_gc = query_cache_entry_count(cache);
    printf("    Entries after GC: %d (removed %d)\n",
           after_gc, before_gc - after_gc);

    /* ── 12. Batch Operations ────────────────────────────────────────── */
    printf("\n── 12. Batch Fetch ──────────────────────────────────────────\n");
    const char *batch_keys[] = {"articles:list", "author:1", "author:2"};
    int batch_count = query_cache_batch_fetch(cache, batch_keys, 3);
    printf("    Batch fetched %d queries.\n", batch_count);

    bool all_ready = query_cache_wait_all(cache, batch_keys, 3, 5000);
    printf("    All ready: %s\n", all_ready ? "yes" : "no");

    /* ── 13. Combine with Zustand ────────────────────────────────────── */
    printf("\n── 13. Zustand + Query Cache Integration ────────────────────\n");

    typedef struct {
        char   article_title[128];
        int    comment_count;
        bool   saving;
    } EditorState;

    EditorState editor = {"New Draft", 0, false};
    ZustandStore *editor_store = zustand_store_create(&editor,
                                                       sizeof(EditorState));

    ZustandMWConfig mw[] = {{ZUSTAND_MW_DEVTOOLS, "editor", NULL}};
    zustand_apply_middleware(editor_store, mw, 1);

    EditorState *es = (EditorState *)zustand_store_get_state(editor_store, NULL);
    printf("    Editor title: '%s', saving=%s\n",
           es->title, es->saving ? "yes" : "no");

    /* ── 14. Subscribe to query ──────────────────────────────────────── */
    printf("\n── 14. Query Listener ────────────────────────────────────────\n");

    int lh = query_entry_subscribe(articles_entry, NULL, NULL);
    printf("    Listener handle: %d\n", lh);
    query_entry_unsubscribe(articles_entry, lh);

    /* ── 15. Error & Retry ───────────────────────────────────────────── */
    printf("\n── 15. Error Handling & Retry ───────────────────────────────\n");

    QueryOptions error_opts = {0};
    error_opts.query_key = "author:999";
    error_opts.stale_time_ms = 5000;
    error_opts.cache_time_ms = 60000;
    error_opts.retry_count = 3;
    error_opts.retry_delay_ms = 500;
    error_opts.retry_backoff = true;

    QueryEntry *error_entry = query_cache_fetch(cache, "author:999",
                                                 fetch_author, &error_opts);
    if (error_entry) {
        printf("    Author 999 fetch status: %d\n",
               query_entry_status(error_entry));
        printf("    Retry attempt: %d\n",
               query_entry_retry_attempt(error_entry));
    }

    /* ── 16. Statistics ──────────────────────────────────────────────── */
    printf("\n── 16. Cache Statistics ──────────────────────────────────────\n");
    printf("    Total entries in cache: %d\n", query_cache_entry_count(cache));
    printf("    Stale entries: %d\n", query_cache_stale_count(cache));
    printf("    Total network calls: %d\n", network_call_count);

    /* ── 17. Remove entry ────────────────────────────────────────────── */
    printf("\n── 17. Remove Entry ──────────────────────────────────────────\n");
    printf("    Entries before remove: %d\n", query_cache_entry_count(cache));
    query_cache_remove(cache, "search:prefetch");
    printf("    Entries after remove: %d\n", query_cache_entry_count(cache));

    /* ── Cleanup ──────────────────────────────────────────────────────── */
    printf("\n── 18. Cleanup ──────────────────────────────────────────────\n");
    infinite_query_destroy(iq);
    zustand_store_destroy(editor_store);
    query_cache_clear(cache);
    query_cache_destroy(cache);

    printf("    All resources freed. Total network calls: %d\n",
           network_call_count);
    printf("\n╔══════════════════════════════════════════════════════════════╗\n");
    printf("║              Query Cache Demo Complete                      ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");

    return 0;
}
