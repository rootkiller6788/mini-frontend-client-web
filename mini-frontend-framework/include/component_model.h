#ifndef COMPONENT_MODEL_H
#define COMPONENT_MODEL_H

#include <stddef.h>
#include <stdbool.h>
#include "virtual_dom.h"

#define COMPONENT_MAX_NAME     64
#define COMPONENT_MAX_PROPS    32
#define COMPONENT_MAX_CONTEXTS 16
#define COMPONENT_MAX_CHILDREN 64

typedef struct Component Component;
typedef struct ComponentProps ComponentProps;
typedef struct ComponentTree ComponentTree;
typedef struct ContextProvider ContextProvider;
typedef struct ContextConsumer ContextConsumer;
typedef struct ErrorBoundary ErrorBoundary;
typedef struct SuspenseFallback SuspenseFallback;
typedef struct LifecycleHooks LifecycleHooks;

typedef struct VNode VNode;
typedef struct HookState HookState;

typedef VNode* (*ComponentFn)(Component* comp, VNode* props);
typedef void   (*LifecycleCallback)(Component* comp);
typedef bool   (*ErrorHandler)(Component* comp, void* error);
typedef bool   (*RouteGuard)(void* params);

struct ComponentProps {
    char keys[COMPONENT_MAX_PROPS][64];
    void* values[COMPONENT_MAX_PROPS];
    int count;
};

struct LifecycleHooks {
    LifecycleCallback on_mount;
    LifecycleCallback on_update;
    LifecycleCallback on_unmount;
    ErrorHandler on_error;
};

struct Component {
    char name[COMPONENT_MAX_NAME];
    int id;
    ComponentFn render;
    ComponentProps props;
    VNode* vnode;
    void* instance;
    HookState* hook_state;
    LifecycleHooks lifecycle;
    Component* parent;
    Component* children[COMPONENT_MAX_CHILDREN];
    int child_count;
    bool is_mounted;
    bool needs_update;
    unsigned int flags;
    void* user_data;
};

struct ContextProvider {
    int context_id;
    void* value;
    Component* owner;
    ContextProvider* parent_provider;
    ContextProvider* children[COMPONENT_MAX_CONTEXTS];
    int child_count;
};

struct ContextConsumer {
    int context_id;
    void* (*selector)(void* context_value);
    Component* subscriber;
    ContextConsumer* next;
};

struct ErrorBoundary {
    Component* component;
    ErrorHandler handler;
    bool has_error;
    void* error_info;
    VNode* fallback_ui;
};

struct SuspenseFallback {
    Component* component;
    bool is_loading;
    void* promise;
    VNode* fallback;
    VNode* resolved;
    Component* owner;
};

struct ComponentTree {
    Component* root;
    int component_count;
    int context_count;
    ContextProvider* context_root;
    ErrorBoundary* error_boundaries;
    int eb_count;
};

Component*  comp_create(const char* name, ComponentFn render);
void        comp_free(Component* comp);
void        comp_free_tree(Component* comp);

void        comp_set_prop(Component* comp, const char* key, void* value);
void*       comp_get_prop(Component* comp, const char* key);
bool        comp_has_prop(Component* comp, const char* key);

void        comp_mount(Component* comp, void* container);
void        comp_update(Component* comp);
void        comp_unmount(Component* comp);
void        comp_force_update(Component* comp);

void        comp_add_child(Component* parent, Component* child);
void        comp_remove_child(Component* parent, Component* child);
Component*  comp_get_child(Component* parent, int index);
Component*  comp_find_by_id(Component* root, int id);

void        comp_set_on_mount(Component* comp, LifecycleCallback cb);
void        comp_set_on_update(Component* comp, LifecycleCallback cb);
void        comp_set_on_unmount(Component* comp, LifecycleCallback cb);
void        comp_set_on_error(Component* comp, ErrorHandler handler);

VNode*      comp_render(Component* comp);

void        comp_tree_init(ComponentTree* tree);
void        comp_tree_set_root(ComponentTree* tree, Component* root);
void        comp_tree_mount(ComponentTree* tree, void* container);
void        comp_tree_unmount(ComponentTree* tree);

int         context_create(void);
void        context_destroy(int context_id);

ContextProvider* context_provider_create(int context_id, void* value, Component* owner);
void             context_provider_free(ContextProvider* provider);
void*            context_get_value(ContextProvider* root, int context_id);
void             context_provide(ContextProvider* parent, int context_id, void* value);
bool             context_subscribe(int context_id, Component* subscriber);

ErrorBoundary*   error_boundary_create(Component* comp, ErrorHandler handler, VNode* fallback_ui);
void             error_boundary_free(ErrorBoundary* eb);
bool             error_boundary_handle(ErrorBoundary* eb, void* error);
VNode*           error_boundary_get_fallback(ErrorBoundary* eb);

SuspenseFallback* suspense_create(Component* comp, VNode* fallback);
void              suspense_free(SuspenseFallback* sf);
void              suspense_resolve(SuspenseFallback* sf, VNode* resolved);
bool              suspense_is_pending(SuspenseFallback* sf);
VNode*            suspense_get_display(SuspenseFallback* sf);

#endif
