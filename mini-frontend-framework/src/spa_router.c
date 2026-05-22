#include "spa_router.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ROUTER_HISTORY_STACK_SIZE 64

static char g_history_stack[ROUTER_HISTORY_STACK_SIZE][ROUTER_URL_MAX_LEN];
static int  g_history_index = -1;
static int  g_history_count = 0;

Router* router_create(RouterMode mode) {
    Router* router = (Router*)calloc(1, sizeof(Router));
    if (!router) return NULL;
    router->mode = mode;
    router->route_count = 0;
    router->current_route = NULL;
    router->current_view = NULL;
    router->not_found_route = NULL;
    router->is_navigating = false;
    router->current_path[0] = '/';
    router->current_path[1] = '\0';
    return router;
}

void router_free(Router* router) {
    if (!router) return;
    if (router->current_view) vdom_free_recursive(router->current_view);
    free(router);
}

int router_add_route(Router* router, const char* path, const char* name,
                     RouteMatchType match_type, RouteComponentFn component) {
    if (!router || router->route_count >= ROUTER_MAX_ROUTES) return -1;
    Route* route = &router->routes[router->route_count];
    strncpy(route->path, path, sizeof(route->path) - 1);
    strncpy(route->name, name, sizeof(route->name) - 1);
    route->match_type = match_type;
    route->component = component;
    route->component_instance = NULL;
    route->child_count = 0;
    route->parent = NULL;
    route->guard_count = 0;
    route->is_lazy = false;
    route->metadata = NULL;
    return router->route_count++;
}

int router_add_route_ex(Router* router, const char* path, const char* name,
                        RouteMatchType match_type, RouteComponentFn component,
                        RouteGuard** guards, int guard_count) {
    int idx = router_add_route(router, path, name, match_type, component);
    if (idx < 0) return idx;
    Route* route = &router->routes[idx];
    for (int i = 0; i < guard_count && i < ROUTER_MAX_GUARDS; i++) {
        route->guards[route->guard_count++] = guards[i];
    }
    return idx;
}

void router_set_not_found(Router* router, const char* path, RouteComponentFn component) {
    if (!router) return;
    Route* nf = (Route*)calloc(1, sizeof(Route));
    if (!nf) return;
    strncpy(nf->path, path, sizeof(nf->path) - 1);
    nf->component = component;
    nf->match_type = ROUTE_WILDCARD;
    router->not_found_route = nf;
}

void router_add_guard(Route* route, const char* name, GuardFn check, const char* redirect) {
    if (!route || route->guard_count >= ROUTER_MAX_GUARDS) return;
    RouteGuard* guard = (RouteGuard*)calloc(1, sizeof(RouteGuard));
    if (!guard) return;
    strncpy(guard->name, name, sizeof(guard->name) - 1);
    guard->check = check;
    if (redirect) strncpy(guard->redirect, redirect, sizeof(guard->redirect) - 1);
    route->guards[route->guard_count++] = guard;
}

void router_add_nested_route(Route* parent, Route* child) {
    if (!parent || !child || parent->child_count >= ROUTER_MAX_NESTED) return;
    parent->children[parent->child_count++] = child;
    child->parent = parent;
}

void router_add_lazy_route(Router* router, const char* path, const char* name,
                           const char* component_path,
                           RouteComponentFn (*loader)(const char*)) {
    int idx = router_add_route(router, path, name, ROUTE_EXACT, NULL);
    if (idx < 0) return;
    Route* route = &router->routes[idx];
    route->is_lazy = true;
    strncpy(route->lazy_path, component_path, sizeof(route->lazy_path) - 1);
}

void router_navigate(Router* router, const char* path) {
    if (!router || !path) return;
    strncpy(router->current_path, path, sizeof(router->current_path) - 1);
    router->is_navigating = true;

    RouteMatch match = router_match(router, path);
    if (match.found) {
        if (match.route->is_lazy && !match.route->component) {
            match.route->component = NULL;
        }
        if (!router_run_guards(match.route, &match.params, router->user_context)) {
            router->is_navigating = false;
            return;
        }
        router->current_route = match.route;
        if (match.route->component) {
            if (router->current_view) vdom_free_recursive(router->current_view);
            router->current_view = match.route->component(&match.params);
            if (router->container) {
                vdom_render_to_dom(router->current_view, router->container);
            }
        }
    } else if (router->not_found_route) {
        router->current_route = router->not_found_route;
        if (router->current_view) vdom_free_recursive(router->current_view);
        router->current_view = router->not_found_route->component(NULL);
        if (router->container) {
            vdom_render_to_dom(router->current_view, router->container);
        }
    }

    if (router->mode == ROUTER_HISTORY)
        router_push_state(path);
    else
        router_hash_set(path);

    router->is_navigating = false;
}

void router_navigate_replace(Router* router, const char* path) {
    if (!router || !path) return;
    strncpy(router->current_path, path, sizeof(router->current_path) - 1);
    if (router->mode == ROUTER_HISTORY)
        router_replace_state(path);
    else
        router_hash_set(path);
}

void router_back(Router* router) {
    if (g_history_index > 0) {
        g_history_index--;
        strncpy(router->current_path, g_history_stack[g_history_index],
                sizeof(router->current_path) - 1);
        router_navigate(router, router->current_path);
    }
}

void router_forward(Router* router) {
    if (g_history_index < g_history_count - 1) {
        g_history_index++;
        strncpy(router->current_path, g_history_stack[g_history_index],
                sizeof(router->current_path) - 1);
        router_navigate(router, router->current_path);
    }
}

void router_push_state(const char* path) {
    if (g_history_index < ROUTER_HISTORY_STACK_SIZE - 1) {
        g_history_index++;
        strncpy(g_history_stack[g_history_index], path, ROUTER_URL_MAX_LEN - 1);
        g_history_count = g_history_index + 1;
    }
}

void router_replace_state(const char* path) {
    if (g_history_index >= 0 && g_history_index < ROUTER_HISTORY_STACK_SIZE) {
        strncpy(g_history_stack[g_history_index], path, ROUTER_URL_MAX_LEN - 1);
    }
}

void router_hash_set(const char* hash) {
    (void)hash;
}

RouteMatch router_match(Router* router, const char* path) {
    RouteMatch result = { .route = NULL, .params = { .count = 0 }, .found = false };

    for (int i = 0; i < router->route_count; i++) {
        Route* route = &router->routes[i];
        RouteParams params = router_parse_params(route->path, path);

        switch (route->match_type) {
            case ROUTE_EXACT:
                if (strcmp(route->path, path) == 0 || params.count > 0) {
                    result.route = route;
                    result.params = params;
                    result.found = true;
                    return result;
                }
                break;
            case ROUTE_PREFIX: {
                size_t len = strlen(route->path);
                if (strncmp(route->path, path, len) == 0) {
                    result.route = route;
                    result.params = params;
                    result.found = true;
                    return result;
                }
                break;
            }
            case ROUTE_PARAM:
                if (params.count > 0) {
                    result.route = route;
                    result.params = params;
                    result.found = true;
                    return result;
                }
                break;
            case ROUTE_WILDCARD:
                result.route = route;
                result.params = params;
                result.found = true;
                return result;
        }
    }
    return result;
}

RouteMatch router_match_nested(Route* parent, const char* path_segment) {
    RouteMatch result = { .route = NULL, .params = { .count = 0 }, .found = false };
    if (!parent) return result;

    for (int i = 0; i < parent->child_count; i++) {
        Route* child = parent->children[i];
        if (strcmp(child->path, path_segment) == 0) {
            result.route = child;
            result.found = true;
            return result;
        }
    }
    return result;
}

RouteParams router_parse_params(const char* pattern, const char* path) {
    RouteParams params = { .count = 0 };
    if (!pattern || !path) return params;

    const char* p = pattern;
    const char* u = path;

    while (*p && *u) {
        if (*p == ':') {
            p++;
            char key[64] = {0};
            int k = 0;
            while (*p && *p != '/' && k < 63) key[k++] = *p++;
            strncpy(params.keys[params.count], key, 63);

            char value[256] = {0};
            int v = 0;
            while (*u && *u != '/' && v < 255) value[v++] = *u++;
            strncpy(params.values[params.count], value, 255);
            params.count++;
        } else if (*p == *u) {
            p++;
            u++;
        } else {
            break;
        }
    }
    return params;
}

char* router_get_param(RouteParams* params, const char* key) {
    if (!params || !key) return NULL;
    for (int i = 0; i < params->count; i++) {
        if (strcmp(params->keys[i], key) == 0)
            return params->values[i];
    }
    return NULL;
}

bool router_params_has(RouteParams* params, const char* key) {
    return router_get_param(params, key) != NULL;
}

const char* router_get_current_path(Router* router) {
    return router ? router->current_path : NULL;
}

Route* router_get_current_route(Router* router) {
    return router ? router->current_route : NULL;
}

VNode* router_render(Router* router) {
    if (!router) return NULL;
    RouteMatch match = router_match(router, router->current_path);
    if (match.found && match.route->component) {
        return match.route->component(&match.params);
    }
    if (router->not_found_route && router->not_found_route->component) {
        return router->not_found_route->component(NULL);
    }
    return NULL;
}

void router_update(Router* router) {
    if (!router) return;
    router_navigate(router, router->current_path);
}

void router_set_container(Router* router, void* container) {
    if (router) router->container = container;
}

void router_set_user_context(Router* router, void* context) {
    if (router) router->user_context = context;
}

bool router_run_guards(Route* route, RouteParams* params, void* context) {
    if (!route) return true;
    for (int i = 0; i < route->guard_count; i++) {
        RouteGuard* guard = route->guards[i];
        if (guard->check && !guard->check(params, context)) {
            return false;
        }
    }
    if (route->parent) {
        return router_run_guards(route->parent, params, context);
    }
    return true;
}

GuardFn router_guard_before_enter(Router* router, const char* path, GuardFn guard) {
    (void)router;
    (void)path;
    return guard;
}

void router_history_push(const char* url) {
    router_push_state(url);
}

void router_history_replace(const char* url) {
    router_replace_state(url);
}

void router_history_back(void) {
    if (g_history_index > 0) g_history_index--;
}

void router_history_forward(void) {
    if (g_history_index < g_history_count - 1) g_history_index++;
}

int router_history_length(void) {
    return g_history_count;
}

void router_on_pop_state(Router* router) {
    if (!router) return;
    router_history_back();
    if (g_history_index >= 0) {
        strncpy(router->current_path, g_history_stack[g_history_index],
                sizeof(router->current_path) - 1);
        router_navigate(router, router->current_path);
    }
}

void router_debug_print(Router* router) {
    if (!router) return;
    printf("Router [mode=%d, routes=%d, current=%s]\n",
           router->mode, router->route_count, router->current_path);
    for (int i = 0; i < router->route_count; i++) {
        printf("  Route %d: %s %s\n", i, router->routes[i].path, router->routes[i].name);
    }
}
