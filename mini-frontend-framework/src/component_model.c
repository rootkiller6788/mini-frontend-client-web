#include "component_model.h"
#include "react_hooks.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int g_component_id_counter = 0;
static int g_context_id_counter = 0;
static ContextProvider* g_context_tree = NULL;

Component* comp_create(const char* name, ComponentFn render) {
    Component* comp = (Component*)calloc(1, sizeof(Component));
    if (!comp) return NULL;
    strncpy(comp->name, name, sizeof(comp->name) - 1);
    comp->id = ++g_component_id_counter;
    comp->render = render;
    comp->props.count = 0;
    return comp;
}

void comp_free(Component* comp) {
    if (!comp) return;
    if (comp->hook_state) {
        hooks_run_cleanups(comp->hook_state);
        hooks_destroy_state(comp->hook_state);
    }
    if (comp->vnode) vdom_free_recursive(comp->vnode);
    free(comp);
}

void comp_free_tree(Component* comp) {
    if (!comp) return;
    for (int i = 0; i < comp->child_count; i++) {
        comp_free_tree(comp->children[i]);
    }
    comp_free(comp);
}

void comp_set_prop(Component* comp, const char* key, void* value) {
    if (!comp || comp->props.count >= COMPONENT_MAX_PROPS) return;
    for (int i = 0; i < comp->props.count; i++) {
        if (strcmp(comp->props.keys[i], key) == 0) {
            comp->props.values[i] = value;
            return;
        }
    }
    strncpy(comp->props.keys[comp->props.count], key, 63);
    comp->props.values[comp->props.count] = value;
    comp->props.count++;
}

void* comp_get_prop(Component* comp, const char* key) {
    if (!comp) return NULL;
    for (int i = 0; i < comp->props.count; i++) {
        if (strcmp(comp->props.keys[i], key) == 0)
            return comp->props.values[i];
    }
    return NULL;
}

bool comp_has_prop(Component* comp, const char* key) {
    return comp_get_prop(comp, key) != NULL;
}

void comp_mount(Component* comp, void* container) {
    if (!comp || comp->is_mounted) return;
    comp->is_mounted = true;
    if (comp->lifecycle.on_mount) {
        comp->lifecycle.on_mount(comp);
    }
    comp->needs_update = true;
    comp_update(comp);
    (void)container;
}

static void rerender_wrapper(void* component) {
    Component* comp = (Component*)component;
    if (comp) {
        comp->needs_update = true;
        comp_update(comp);
    }
}

void comp_update(Component* comp) {
    if (!comp || !comp->is_mounted || !comp->needs_update) return;
    comp->needs_update = false;

    if (!comp->hook_state) {
        comp->hook_state = hooks_create_state(comp, rerender_wrapper);
    }

    hooks_begin_render(comp->hook_state);
    VNode* new_vnode = comp->render(comp, NULL);
    hooks_end_render(comp->hook_state);

    if (comp->vnode) {
        vdom_diff(comp->vnode, new_vnode, NULL);
        vdom_free_recursive(comp->vnode);
    }
    comp->vnode = new_vnode;

    hooks_run_effects(comp->hook_state);

    if (comp->lifecycle.on_update) {
        comp->lifecycle.on_update(comp);
    }
}

void comp_unmount(Component* comp) {
    if (!comp || !comp->is_mounted) return;
    comp->is_mounted = false;
    if (comp->lifecycle.on_unmount) {
        comp->lifecycle.on_unmount(comp);
    }
    if (comp->hook_state) {
        hooks_run_cleanups(comp->hook_state);
    }
    if (comp->vnode) {
        vdom_free_recursive(comp->vnode);
        comp->vnode = NULL;
    }
}

void comp_force_update(Component* comp) {
    if (!comp) return;
    comp->needs_update = true;
    comp_update(comp);
}

void comp_add_child(Component* parent, Component* child) {
    if (!parent || !child || parent->child_count >= COMPONENT_MAX_CHILDREN) return;
    parent->children[parent->child_count++] = child;
    child->parent = parent;
}

void comp_remove_child(Component* parent, Component* child) {
    if (!parent || !child) return;
    for (int i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            memmove(&parent->children[i], &parent->children[i + 1],
                    (parent->child_count - i - 1) * sizeof(Component*));
            parent->child_count--;
            child->parent = NULL;
            return;
        }
    }
}

Component* comp_get_child(Component* parent, int index) {
    if (!parent || index < 0 || index >= parent->child_count) return NULL;
    return parent->children[index];
}

Component* comp_find_by_id(Component* root, int id) {
    if (!root) return NULL;
    if (root->id == id) return root;
    for (int i = 0; i < root->child_count; i++) {
        Component* found = comp_find_by_id(root->children[i], id);
        if (found) return found;
    }
    return NULL;
}

void comp_set_on_mount(Component* comp, LifecycleCallback cb) { if (comp) comp->lifecycle.on_mount = cb; }
void comp_set_on_update(Component* comp, LifecycleCallback cb) { if (comp) comp->lifecycle.on_update = cb; }
void comp_set_on_unmount(Component* comp, LifecycleCallback cb) { if (comp) comp->lifecycle.on_unmount = cb; }
void comp_set_on_error(Component* comp, ErrorHandler handler) { if (comp) comp->lifecycle.on_error = handler; }

VNode* comp_render(Component* comp) {
    if (!comp || !comp->render) return NULL;
    return comp->render(comp, NULL);
}

void comp_tree_init(ComponentTree* tree) {
    tree->root = NULL;
    tree->component_count = 0;
    tree->context_count = 0;
    tree->context_root = NULL;
    tree->error_boundaries = NULL;
    tree->eb_count = 0;
}

void comp_tree_set_root(ComponentTree* tree, Component* root) {
    tree->root = root;
    tree->component_count = 1;
}

void comp_tree_mount(ComponentTree* tree, void* container) {
    if (tree->root) comp_mount(tree->root, container);
}

void comp_tree_unmount(ComponentTree* tree) {
    if (tree->root) comp_unmount(tree->root);
}

int context_create(void) {
    return ++g_context_id_counter;
}

void context_destroy(int context_id) {
    (void)context_id;
}

ContextProvider* context_provider_create(int context_id, void* value, Component* owner) {
    ContextProvider* provider = (ContextProvider*)calloc(1, sizeof(ContextProvider));
    if (!provider) return NULL;
    provider->context_id = context_id;
    provider->value = value;
    provider->owner = owner;
    if (g_context_tree) {
        provider->parent_provider = g_context_tree;
        provider->parent_provider->children[provider->parent_provider->child_count++] = provider;
    }
    g_context_tree = provider;
    return provider;
}

void context_provider_free(ContextProvider* provider) {
    if (!provider) return;
    for (int i = 0; i < provider->child_count; i++) {
        context_provider_free(provider->children[i]);
    }
    free(provider);
}

void* context_get_value(ContextProvider* root, int context_id) {
    ContextProvider* current = root ? root : g_context_tree;
    while (current) {
        if (current->context_id == context_id)
            return current->value;
        current = current->parent_provider;
    }
    return NULL;
}

void context_provide(ContextProvider* parent, int context_id, void* value) {
    ContextProvider* provider = context_provider_create(context_id, value, NULL);
    if (parent) {
        provider->parent_provider = parent;
        parent->children[parent->child_count++] = provider;
    }
}

bool context_subscribe(int context_id, Component* subscriber) {
    (void)context_id;
    (void)subscriber;
    return true;
}

ErrorBoundary* error_boundary_create(Component* comp, ErrorHandler handler, VNode* fallback_ui) {
    ErrorBoundary* eb = (ErrorBoundary*)calloc(1, sizeof(ErrorBoundary));
    if (!eb) return NULL;
    eb->component = comp;
    eb->handler = handler;
    eb->fallback_ui = fallback_ui;
    eb->has_error = false;
    return eb;
}

void error_boundary_free(ErrorBoundary* eb) {
    if (!eb) return;
    free(eb);
}

bool error_boundary_handle(ErrorBoundary* eb, void* error) {
    if (!eb) return false;
    eb->has_error = true;
    eb->error_info = error;
    if (eb->handler) {
        return eb->handler(eb->component, error);
    }
    return false;
}

VNode* error_boundary_get_fallback(ErrorBoundary* eb) {
    if (!eb) return NULL;
    return eb->has_error ? eb->fallback_ui : NULL;
}

SuspenseFallback* suspense_create(Component* comp, VNode* fallback) {
    SuspenseFallback* sf = (SuspenseFallback*)calloc(1, sizeof(SuspenseFallback));
    if (!sf) return NULL;
    sf->component = comp;
    sf->fallback = fallback;
    sf->is_loading = true;
    return sf;
}

void suspense_free(SuspenseFallback* sf) {
    if (!sf) return;
    free(sf);
}

void suspense_resolve(SuspenseFallback* sf, VNode* resolved) {
    if (!sf) return;
    sf->resolved = resolved;
    sf->is_loading = false;
}

bool suspense_is_pending(SuspenseFallback* sf) {
    return sf ? sf->is_loading : false;
}

VNode* suspense_get_display(SuspenseFallback* sf) {
    if (!sf) return NULL;
    return sf->is_loading ? sf->fallback : sf->resolved;
}
