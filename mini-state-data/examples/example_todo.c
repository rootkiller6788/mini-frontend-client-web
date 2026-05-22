#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "redux_store.h"
#include "mobx_observable.h"
#include "zustand_sim.h"
#include "immutable_store.h"

/* ── Demo: Todo application using Redux + Zustand + Immutable ──────────── */

typedef struct {
    char  title[128];
    int   id;
    bool  completed;
} Todo;

typedef struct {
    Todo *items;
    int   count;
    int   capacity;
    int   next_id;
} TodoState;

static TodoState *todo_state_create(void)
{
    TodoState *ts = calloc(1, sizeof(TodoState));
    ts->capacity = 16;
    ts->items = calloc(ts->capacity, sizeof(Todo));
    ts->next_id = 1;
    return ts;
}

static void todo_state_destroy(void *state)
{
    TodoState *ts = (TodoState *)state;
    free(ts->items);
    free(ts);
}

static void *todo_reducer(void *state, const ReduxAction *action,
                          size_t *out_size)
{
    TodoState *ts = (TodoState *)state;
    if (!ts) return NULL;

    if (strcmp(action->type, "TODO_ADD") == 0) {
        if (action->payload && action->payload_size > 0) {
            if (ts->count >= ts->capacity) {
                ts->capacity *= 2;
                ts->items = realloc(ts->items, ts->capacity * sizeof(Todo));
            }
            Todo *t = &ts->items[ts->count++];
            memcpy(t->title, action->payload,
                   action->payload_size < 128 ? action->payload_size : 127);
            t->title[127] = '\0';
            t->id = ts->next_id++;
            t->completed = false;
        }
    } else if (strcmp(action->type, "TODO_TOGGLE") == 0) {
        if (action->payload) {
            int id = *(int *)action->payload;
            for (int i = 0; i < ts->count; i++) {
                if (ts->items[i].id == id) {
                    ts->items[i].completed = !ts->items[i].completed;
                    break;
                }
            }
        }
    } else if (strcmp(action->type, "TODO_REMOVE") == 0) {
        if (action->payload) {
            int id = *(int *)action->payload;
            for (int i = 0; i < ts->count; i++) {
                if (ts->items[i].id == id) {
                    ts->items[i] = ts->items[ts->count - 1];
                    ts->count--;
                    break;
                }
            }
        }
    }

    *out_size = sizeof(TodoState);
    return ts;
}

/* ── Zustand demo for UI state ──────────────────────────────────────────── */

typedef struct {
    char filter[16];   /* "all", "active", "completed" */
} UIState;

/* ── MobX demo for computed ─────────────────────────────────────────────── */

static MobxObservable *g_todo_count = NULL;

static MobxValue compute_count(void *userdata)
{
    TodoState *ts = (TodoState *)userdata;
    return MOBX_INT(ts ? ts->count : 0);
}

/* ── Immutable demo for normalized store ─────────────────────────────────── */

static ImmutNormalized *g_normalized = NULL;

/* ── Main demo ──────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== mini-state-data: Todo Application Demo ===\n\n");

    /* 1. Redux store for todos */
    printf("--- Redux Store: Todo Management ---\n");
    TodoState *initial = todo_state_create();
    ReduxReducer reducer = {todo_reducer, NULL, todo_state_destroy};

    ReduxStore *store = redux_store_create(initial, sizeof(TodoState), reducer);
    if (!store) { printf("Failed to create store\n"); return 1; }

    /* Add logger middleware */
    redux_store_add_middleware(store, redux_middleware_logger());

    /* Dispatch add actions */
    ReduxAction a1 = REDUX_ACTION_PAYLOAD("TODO_ADD", "Buy milk", 9);
    redux_store_dispatch(store, &a1);

    ReduxAction a2 = REDUX_ACTION_PAYLOAD("TODO_ADD", "Write code", 11);
    redux_store_dispatch(store, &a2);

    ReduxAction a3 = REDUX_ACTION_PAYLOAD("TODO_ADD", "Read book", 10);
    redux_store_dispatch(store, &a3);

    /* Toggle first todo */
    size_t sz;
    TodoState *ts = (TodoState *)redux_store_get_state(store, &sz);
    if (ts && ts->count > 0) {
        int toggle_id = ts->items[0].id;
        ReduxAction a4 = REDUX_ACTION_PAYLOAD("TODO_TOGGLE", &toggle_id, sizeof(int));
        redux_store_dispatch(store, &a4);
    }

    /* Print state */
    ts = (TodoState *)redux_store_get_state(store, &sz);
    printf("Todos (%d items):\n", ts ? ts->count : 0);
    if (ts) {
        for (int i = 0; i < ts->count; i++) {
            printf("  [%c] %d: %s\n",
                   ts->items[i].completed ? 'x' : ' ',
                   ts->items[i].id, ts->items[i].title);
        }
    }

    /* 2. MobX: computed values */
    printf("\n--- MobX: Computed & Observable ---\n");
    g_todo_count = mobx_observable_create("todoCount", MOBX_INT(ts ? ts->count : 0));

    MobxComputed *description = mobx_computed_create("desc", compute_count, ts);
    MobxValue desc_val = mobx_computed_get(description);
    printf("Computed description: %lld todos\n", (long long)desc_val.data.i);

    /* Transaction batch update */
    MOBX_RAW({
        mobx_observable_set(g_todo_count, MOBX_INT(10));
        printf("Inside transaction: depth = %d\n", mobx_transaction_depth());
    });

    /* Observable array */
    MobxArray *tags = mobx_array_create();
    mobx_array_push(tags, MOBX_STRING("urgent"));
    mobx_array_push(tags, MOBX_STRING("personal"));
    mobx_array_push(tags, MOBX_STRING("work"));
    printf("Tags: ");
    for (int i = 0; i < mobx_array_length(tags); i++) {
        printf("%s ", mobx_array_get(tags, i).data.s);
    }
    printf("\n");

    /* Observable map */
    MobxMap *settings = mobx_map_create();
    mobx_map_set(settings, "theme", MOBX_STRING("dark"));
    mobx_map_set(settings, "lang", MOBX_STRING("en"));
    printf("Settings: theme=%s, lang=%s\n",
           mobx_map_get(settings, "theme").data.s,
           mobx_map_get(settings, "lang").data.s);

    /* 3. Zustand: UI state */
    printf("\n--- Zustand: UI State Management ---\n");
    UIState ui = {0};
    strcpy(ui.filter, "all");
    ZustandStore *ui_store = zustand_store_create(&ui, sizeof(UIState));

    ZustandDraft *draft = zustand_draft_create(&ui, sizeof(UIState));
    zustand_draft_set(draft, 0, "active", 7);
    size_t new_sz;
    void *new_ui = zustand_draft_finish(draft, &new_sz);
    zustand_store_set_state(ui_store, new_ui, new_sz, true);
    free(new_ui);

    UIState *current_ui = (UIState *)zustand_store_get_state(ui_store, NULL);
    if (current_ui) printf("Current filter: %s\n", current_ui->filter);

    /* 4. Immutable normalized store */
    printf("\n--- Immutable Store: Normalized Entities ---\n");
    NormalizedSchema schemas[] = {
        {"todo", "id", NULL, 0},
        {"user", "id", NULL, 0},
    };
    g_normalized = immut_normalized_create(schemas, 2);

    Todo t = {"Hello world", 1, false};
    immut_normalized_upsert(g_normalized, "todo", "1", &t, sizeof(Todo));

    Todo *found = immut_normalized_get(g_normalized, "todo", "1", &sz);
    if (found) printf("Found todo: %s (id=%d)\n", found->title, found->id);

    /* 5. Immutable list operations */
    printf("\n--- Immutable List: Functional Operations ---\n");
    ImmutList *list = immut_list_create();
    char *items[] = {"alpha", "beta", "gamma", "delta"};
    for (int i = 0; i < 4; i++) {
        immut_list_push(list, items[i], strlen(items[i]) + 1);
    }
    printf("List size: %d\n", immut_list_size(list));

    for (int i = 0; i < immut_list_size(list); i++) {
        void *item = immut_list_get(list, i, NULL);
        printf("  [%d]: %s\n", i, item ? (char *)item : "null");
    }

    /* 6. DevTools: time-travel */
    printf("\n--- Redux DevTools: Time-Travel ---\n");
    ReduxDevTools *devtools = redux_devtools_create(100);

    TodoState *snap_ts = (TodoState *)redux_store_get_state(store, &sz);
    redux_devtools_record(devtools, snap_ts, sz, "INITIAL");
    printf("DevTools history: %d entries\n", redux_devtools_history_count(devtools));

    /* Cleanup */
    printf("\n--- Cleanup ---\n");
    mobx_observable_destroy(g_todo_count);
    mobx_computed_destroy(description);
    mobx_array_destroy(tags);
    mobx_map_destroy(settings);
    zustand_store_destroy(ui_store);
    immut_normalized_destroy(g_normalized);
    immut_list_destroy(list);
    redux_devtools_destroy(devtools);
    redux_store_destroy(store);

    printf("Done.\n");
    return 0;
}
