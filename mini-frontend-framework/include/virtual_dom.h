#ifndef VIRTUAL_DOM_H
#define VIRTUAL_DOM_H

#include <stddef.h>
#include <stdbool.h>

#define VDOM_MAX_CHILDREN 256
#define VDOM_MAX_PROPS   64
#define VDOM_BATCH_SIZE  128

typedef enum {
    VNODE_ELEMENT,
    VNODE_TEXT,
    VNODE_FRAGMENT,
    VNODE_COMPONENT
} VNodeType;

typedef enum {
    VPATCH_NONE,
    VPATCH_CREATE,
    VPATCH_REMOVE,
    VPATCH_REPLACE,
    VPATCH_TEXT,
    VPATCH_PROPS,
    VPATCH_REORDER
} VPatchType;

typedef struct VNode VNode;
typedef struct VProp VProp;
typedef struct VPatch VPatch;
typedef struct VNodeList VNodeList;
typedef struct UpdateBatch UpdateBatch;
typedef struct Component Component;

typedef VNode* (*ComponentRender)(Component*, VNode*);

struct VProp {
    char key[64];
    char value[256];
    bool is_event;
};

struct VNode {
    VNodeType type;
    char tag[32];
    char text[1024];
    char key[64];
    VProp props[VDOM_MAX_PROPS];
    int prop_count;
    VNode* children[VDOM_MAX_CHILDREN];
    int child_count;
    VNode* parent;
    Component* component;
    ComponentRender render_fn;
    void* dom_ref;
    unsigned int flags;
};

struct VNodeList {
    VNode** items;
    int count;
    int capacity;
};

struct VPatch {
    VPatchType type;
    VNode* target;
    VNode* replacement;
    void* dom_parent;
    int index;
    VProp* new_props;
    int prop_count;
    char new_text[1024];
};

struct UpdateBatch {
    VPatch* patches[VDOM_BATCH_SIZE];
    int count;
    bool pending;
};

VNode*  vdom_create_element(const char* tag);
VNode*  vdom_create_text(const char* text);
VNode*  vdom_create_fragment(void);
VNode*  vdom_create_component(const char* name, ComponentRender render_fn);

void    vdom_set_prop(VNode* node, const char* key, const char* value);
void    vdom_set_event(VNode* node, const char* event, const char* handler);
void    vdom_append_child(VNode* parent, VNode* child);
void    vdom_remove_child(VNode* parent, VNode* child);
void    vdom_replace_child(VNode* parent, VNode* old_child, VNode* new_child);
void    vdom_set_text(VNode* node, const char* text);
void    vdom_set_key(VNode* node, const char* key);

VNode*  vdom_clone(VNode* node);
void    vdom_free(VNode* node);
void    vdom_free_recursive(VNode* node);

void    vdom_diff(VNode* old_tree, VNode* new_tree, void* dom_parent);
void    vdom_patch_children(VNode* old_parent, VNode* new_parent, void* dom_parent);
void    vdom_patch_props(VNode* old_node, VNode* new_node, void* dom_parent);
void    vdom_keyed_reconciliation(VNodeList* old_children, VNodeList* new_children, void* dom_parent, VNode* parent_node);

int     vdom_find_child_by_key(VNode* parent, const char* key);

void    vdom_patch_apply(VPatch* patch);
void    vdom_patch(VNode* old_tree, VNode* new_tree);

void    vdom_batch_begin(void);
void    vdom_batch_end(void);
void    vdom_batch_schedule(VPatch* patch);
void    vdom_batch_flush(void);
bool    vdom_batch_is_pending(void);

void    vdom_dom_create_element(VNode* node, void* dom_parent);
void    vdom_dom_remove_node(VNode* node);
void    vdom_dom_update_text(VNode* node, const char* text);
void    vdom_dom_update_props(VNode* node, VProp* props, int count);
void    vdom_dom_move_node(VNode* node, void* new_parent, int index);

void    vdom_node_list_init(VNodeList* list, int capacity);
void    vdom_node_list_add(VNodeList* list, VNode* node);
void    vdom_node_list_free(VNodeList* list);
VNode*  vdom_node_list_get(VNodeList* list, int index);

void    vdom_render_to_dom(VNode* root, void* container);
void    vdom_update_dom(VNode* old_root, VNode* new_root, void* container);

const char* vdom_node_type_name(VNodeType type);
void    vdom_debug_print(VNode* node, int depth);

#endif
