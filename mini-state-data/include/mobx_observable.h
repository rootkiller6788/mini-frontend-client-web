#ifndef MOBX_OBSERVABLE_H
#define MOBX_OBSERVABLE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Core Observable ───────────────────────────────────────────────────── */

typedef struct MobxObservable MobxObservable;
typedef struct MobxComputed   MobxComputed;
typedef struct MobxReaction   MobxReaction;
typedef struct MobxTransaction MobxTransaction;

typedef enum {
    MOBX_TYPE_INT,
    MOBX_TYPE_DOUBLE,
    MOBX_TYPE_STRING,
    MOBX_TYPE_PTR,
    MOBX_TYPE_BOOL,
    MOBX_TYPE_ARRAY,
    MOBX_TYPE_MAP,
    MOBX_TYPE_SET
} MobxValueType;

typedef struct MobxValue {
    MobxValueType type;
    union {
        int64_t    i;
        double     d;
        char      *s;
        void      *p;
        bool       b;
    } data;
    size_t size;
} MobxValue;

/* ── Observable ─────────────────────────────────────────────────────────── */

MobxObservable *mobx_observable_create(const char *name, MobxValue initial);
void            mobx_observable_destroy(MobxObservable *obs);

MobxValue       mobx_observable_get(MobxObservable *obs);
void            mobx_observable_set(MobxObservable *obs, MobxValue value);

const char     *mobx_observable_name(MobxObservable *obs);
void           *mobx_observable_userdata(MobxObservable *obs);
void            mobx_observable_set_userdata(MobxObservable *obs, void *ud);

#define MOBX_INT(V)    ((MobxValue){.type=MOBX_TYPE_INT, .data.i=(V)})
#define MOBX_DOUBLE(V) ((MobxValue){.type=MOBX_TYPE_DOUBLE, .data.d=(V)})
#define MOBX_STRING(V) ((MobxValue){.type=MOBX_TYPE_STRING, .data.s=(char*)(V)})
#define MOBX_PTR(V,S)  ((MobxValue){.type=MOBX_TYPE_PTR, .data.p=(V), .size=(S)})
#define MOBX_BOOL(V)   ((MobxValue){.type=MOBX_TYPE_BOOL, .data.b=(V)})

/* ── Autorun ────────────────────────────────────────────────────────────── */

typedef struct MobxAutorun MobxAutorun;
typedef void (*MobxAutorunFn)(MobxAutorun *ar, void *userdata);

MobxAutorun *mobx_autorun_create(MobxAutorunFn fn, void *userdata);
void         mobx_autorun_destroy(MobxAutorun *ar);
void         mobx_autorun_schedule(MobxAutorun *ar);
bool         mobx_autorun_is_scheduled(MobxAutorun *ar);

void         mobx_autorun_track(MobxAutorun *ar, MobxObservable *obs);
void         mobx_autorun_untrack(MobxAutorun *ar, MobxObservable *obs);
int          mobx_autorun_dependency_count(MobxAutorun *ar);

/* ── Computed ───────────────────────────────────────────────────────────── */

typedef MobxValue (*MobxComputedFn)(void *userdata);

MobxComputed *mobx_computed_create(const char *name, MobxComputedFn fn,
                                   void *userdata);
void          mobx_computed_destroy(MobxComputed *comp);
MobxValue     mobx_computed_get(MobxComputed *comp);
bool          mobx_computed_is_stale(MobxComputed *comp);
void          mobx_computed_invalidate(MobxComputed *comp);
const char   *mobx_computed_name(MobxComputed *comp);

/* ── Action Boundary ────────────────────────────────────────────────────── */

typedef struct MobxAction MobxAction;

MobxAction *mobx_action_begin(const char *name);
void        mobx_action_end(MobxAction *action);
bool        mobx_action_is_running(void);
const char *mobx_action_current_name(void);

#define MOBX_ACTION_BLOCK(NAME, BODY) do { \
    MobxAction *_mobx_act = mobx_action_begin(NAME); \
    BODY \
    mobx_action_end(_mobx_act); \
} while(0)

/* ── Reactions ──────────────────────────────────────────────────────────── */

typedef void (*MobxSideEffectFn)(void *userdata);
typedef bool (*MobxWhenPredicate)(void *userdata);

MobxReaction *mobx_reaction_create(MobxAutorunFn track_fn,
                                   MobxSideEffectFn effect_fn,
                                   void *userdata);
void          mobx_reaction_destroy(MobxReaction *reaction);
void          mobx_reaction_run(MobxReaction *reaction);
void          mobx_reaction_dispose(MobxReaction *reaction);

void          mobx_when(MobxWhenPredicate pred, MobxSideEffectFn effect,
                        void *userdata);

/* ── Observable Collections ─────────────────────────────────────────────── */

typedef struct MobxArray MobxArray;
typedef struct MobxMap   MobxMap;
typedef struct MobxSet   MobxSet;

/* Observable Array */
MobxArray *mobx_array_create(void);
void       mobx_array_destroy(MobxArray *arr);
int        mobx_array_length(MobxArray *arr);
MobxValue  mobx_array_get(MobxArray *arr, int index);
void       mobx_array_set(MobxArray *arr, int index, MobxValue value);
void       mobx_array_push(MobxArray *arr, MobxValue value);
MobxValue  mobx_array_pop(MobxArray *arr);
void       mobx_array_insert(MobxArray *arr, int index, MobxValue value);
MobxValue  mobx_array_remove(MobxArray *arr, int index);
void       mobx_array_clear(MobxArray *arr);
void      *mobx_array_data(MobxArray *arr, int *count);

/* Observable Map */
MobxMap   *mobx_map_create(void);
void       mobx_map_destroy(MobxMap *map);
int        mobx_map_size(MobxMap *map);
bool       mobx_map_has(MobxMap *map, const char *key);
MobxValue  mobx_map_get(MobxMap *map, const char *key);
void       mobx_map_set(MobxMap *map, const char *key, MobxValue value);
void       mobx_map_delete(MobxMap *map, const char *key);
void       mobx_map_clear(MobxMap *map);

typedef void (*MobxMapIterFn)(const char *key, MobxValue value, void *userdata);
void       mobx_map_foreach(MobxMap *map, MobxMapIterFn fn, void *userdata);

/* Observable Set */
MobxSet   *mobx_set_create(void);
void       mobx_set_destroy(MobxSet *set);
int        mobx_set_size(MobxSet *set);
bool       mobx_set_has(MobxSet *set, MobxValue value);
void       mobx_set_add(MobxSet *set, MobxValue value);
void       mobx_set_delete(MobxSet *set, MobxValue value);
void       mobx_set_clear(MobxSet *set);

typedef void (*MobxSetIterFn)(MobxValue value, void *userdata);
void       mobx_set_foreach(MobxSet *set, MobxSetIterFn fn, void *userdata);

/* ── Transaction ────────────────────────────────────────────────────────── */

MobxTransaction *mobx_transaction_begin(void);
void             mobx_transaction_end(MobxTransaction *tx);
bool             mobx_transaction_is_active(void);
int              mobx_transaction_depth(void);

#define MOBX_RAW(CODE) do { \
    MobxTransaction *_mobx_tx = mobx_transaction_begin(); \
    CODE \
    mobx_transaction_end(_mobx_tx); \
} while(0)

/* ── Global Batch ───────────────────────────────────────────────────────── */

void mobx_batch_begin(void);
void mobx_batch_end(void);
void mobx_batch_flush(void);

#ifdef __cplusplus
}
#endif

#endif /* MOBX_OBSERVABLE_H */
