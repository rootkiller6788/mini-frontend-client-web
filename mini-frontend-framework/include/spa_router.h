#ifndef SPA_ROUTER_H
#define SPA_ROUTER_H

#include <stddef.h>
#include <stdbool.h>

#define ROUTER_MAX_ROUTES      64
#define ROUTER_MAX_PARAMS      16
#define ROUTER_MAX_GUARDS      8
#define ROUTER_MAX_NESTED      8
#define ROUTER_PATH_MAX_LEN    256
#define ROUTER_URL_MAX_LEN     1024

typedef enum {
    ROUTE_EXACT,
    ROUTE_PREFIX,
    ROUTE_PARAM,
    ROUTE_WILDCARD
} RouteMatchType;

typedef enum {
    ROUTER_HISTORY,
    ROUTER_HASH
} RouterMode;

typedef struct Route Route;
typedef struct RouteParams RouteParams;
typedef struct RouteGuard RouteGuard;
typedef struct Router Router;
typedef struct RouteMatch RouteMatch;
typedef struct LazyRoute LazyRoute;

typedef struct Component Component;
typedef struct VNode VNode;
typedef VNode* (*RouteComponentFn)(void* params);
typedef bool  (*GuardFn)(void* params, void* context);

struct RouteParams {
    char keys[ROUTER_MAX_PARAMS][64];
    char values[ROUTER_MAX_PARAMS][256];
    int count;
};

struct RouteGuard {
    char name[64];
    GuardFn check;
    char redirect[ROUTER_PATH_MAX_LEN];
};

struct Route {
    char path[ROUTER_PATH_MAX_LEN];
    char name[64];
    RouteMatchType match_type;
    RouteComponentFn component;
    Component* component_instance;
    Route* children[ROUTER_MAX_NESTED];
    int child_count;
    Route* parent;
    RouteGuard* guards[ROUTER_MAX_GUARDS];
    int guard_count;
    bool is_lazy;
    char lazy_path[ROUTER_PATH_MAX_LEN];
    void* metadata;
};

struct LazyRoute {
    char path[ROUTER_PATH_MAX_LEN];
    char component_path[ROUTER_PATH_MAX_LEN];
    RouteComponentFn (*loader)(const char* path);
    bool is_loaded;
    RouteComponentFn component;
};

struct RouteMatch {
    Route* route;
    RouteParams params;
    bool found;
};

struct Router {
    Route routes[ROUTER_MAX_ROUTES];
    int route_count;
    Route* current_route;
    char current_path[ROUTER_URL_MAX_LEN];
    RouterMode mode;
    void* container;
    Component* root_component;
    VNode* current_view;
    Route* not_found_route;
    void* user_context;
    bool is_navigating;
};

Router*      router_create(RouterMode mode);
void         router_free(Router* router);

int          router_add_route(Router* router, const char* path, const char* name,
                              RouteMatchType match_type, RouteComponentFn component);
int          router_add_route_ex(Router* router, const char* path, const char* name,
                                 RouteMatchType match_type, RouteComponentFn component,
                                 RouteGuard** guards, int guard_count);
void         router_set_not_found(Router* router, const char* path, RouteComponentFn component);

void         router_add_guard(Route* route, const char* name, GuardFn check, const char* redirect);
void         router_add_nested_route(Route* parent, Route* child);
void         router_add_lazy_route(Router* router, const char* path, const char* name,
                                   const char* component_path, RouteComponentFn (*loader)(const char*));

void         router_navigate(Router* router, const char* path);
void         router_navigate_replace(Router* router, const char* path);
void         router_back(Router* router);
void         router_forward(Router* router);

void         router_push_state(const char* path);
void         router_replace_state(const char* path);
void         router_hash_set(const char* hash);

RouteMatch  router_match(Router* router, const char* path);
RouteMatch  router_match_nested(Route* parent, const char* path_segment);

RouteParams router_parse_params(const char* pattern, const char* path);
char*       router_get_param(RouteParams* params, const char* key);
bool        router_params_has(RouteParams* params, const char* key);

const char* router_get_current_path(Router* router);
Route*      router_get_current_route(Router* router);
VNode*      router_render(Router* router);
void        router_update(Router* router);

void        router_set_container(Router* router, void* container);
void        router_set_user_context(Router* router, void* context);

bool        router_run_guards(Route* route, RouteParams* params, void* context);
GuardFn     router_guard_before_enter(Router* router, const char* path, GuardFn guard);

void        router_history_push(const char* url);
void        router_history_replace(const char* url);
void        router_history_back(void);
void        router_history_forward(void);
int         router_history_length(void);

void        router_on_pop_state(Router* router);

void        router_debug_print(Router* router);

#endif
