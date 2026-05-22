#include "virtual_dom.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static UpdateBatch g_batch = { .count = 0, .pending = false };

VNode* vdom_create_element(const char* tag) {
    VNode* node = (VNode*)calloc(1, sizeof(VNode));
    if (!node) return NULL;
    node->type = VNODE_ELEMENT;
    strncpy(node->tag, tag, sizeof(node->tag) - 1);
    node->flags = 0;
    return node;
}

VNode* vdom_create_text(const char* text) {
    VNode* node = (VNode*)calloc(1, sizeof(VNode));
    if (!node) return NULL;
    node->type = VNODE_TEXT;
    strncpy(node->text, text, sizeof(node->text) - 1);
    return node;
}

VNode* vdom_create_fragment(void) {
    VNode* node = (VNode*)calloc(1, sizeof(VNode));
    if (!node) return NULL;
    node->type = VNODE_FRAGMENT;
    return node;
}

VNode* vdom_create_component(const char* name, ComponentRender render_fn) {
    VNode* node = (VNode*)calloc(1, sizeof(VNode));
    if (!node) return NULL;
    node->type = VNODE_COMPONENT;
    strncpy(node->tag, name, sizeof(node->tag) - 1);
    node->render_fn = render_fn;
    return node;
}

void vdom_set_prop(VNode* node, const char* key, const char* value) {
    if (!node || node->prop_count >= VDOM_MAX_PROPS) return;
    VProp* prop = &node->props[node->prop_count++];
    strncpy(prop->key, key, sizeof(prop->key) - 1);
    strncpy(prop->value, value, sizeof(prop->value) - 1);
    prop->is_event = false;
}

void vdom_set_event(VNode* node, const char* event, const char* handler) {
    if (!node || node->prop_count >= VDOM_MAX_PROPS) return;
    VProp* prop = &node->props[node->prop_count++];
    strncpy(prop->key, event, sizeof(prop->key) - 1);
    strncpy(prop->value, handler, sizeof(prop->value) - 1);
    prop->is_event = true;
}

void vdom_append_child(VNode* parent, VNode* child) {
    if (!parent || !child || parent->child_count >= VDOM_MAX_CHILDREN) return;
    parent->children[parent->child_count++] = child;
    child->parent = parent;
}

void vdom_remove_child(VNode* parent, VNode* child) {
    if (!parent || !child) return;
    for (int i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            memmove(&parent->children[i], &parent->children[i + 1],
                    (parent->child_count - i - 1) * sizeof(VNode*));
            parent->child_count--;
            child->parent = NULL;
            return;
        }
    }
}

void vdom_replace_child(VNode* parent, VNode* old_child, VNode* new_child) {
    if (!parent || !old_child || !new_child) return;
    for (int i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == old_child) {
            parent->children[i] = new_child;
            new_child->parent = parent;
            old_child->parent = NULL;
            return;
        }
    }
}

void vdom_set_text(VNode* node, const char* text) {
    if (!node) return;
    strncpy(node->text, text, sizeof(node->text) - 1);
}

void vdom_set_key(VNode* node, const char* key) {
    if (!node) return;
    strncpy(node->key, key, sizeof(node->key) - 1);
}

VNode* vdom_clone(VNode* node) {
    if (!node) return NULL;
    VNode* clone = (VNode*)calloc(1, sizeof(VNode));
    if (!clone) return NULL;
    memcpy(clone, node, sizeof(VNode));
    for (int i = 0; i < node->child_count; i++) {
        VNode* child_clone = vdom_clone(node->children[i]);
        if (child_clone) {
            clone->children[i] = child_clone;
            child_clone->parent = clone;
        }
    }
    return clone;
}

void vdom_free(VNode* node) {
    if (!node) return;
    if (node->type == VNODE_COMPONENT && node->component) {
        comp_free(node->component);
    }
    free(node);
}

void vdom_free_recursive(VNode* node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) {
        vdom_free_recursive(node->children[i]);
    }
    vdom_free(node);
}

void vdom_diff(VNode* old_tree, VNode* new_tree, void* dom_parent) {
    if (!old_tree && !new_tree) return;
    if (!old_tree && new_tree) {
        vdom_dom_create_element(new_tree, dom_parent);
        return;
    }
    if (old_tree && !new_tree) {
        vdom_dom_remove_node(old_tree);
        return;
    }
    if (old_tree->type != new_tree->type || strcmp(old_tree->tag, new_tree->tag) != 0) {
        vdom_dom_remove_node(old_tree);
        vdom_dom_create_element(new_tree, dom_parent);
        return;
    }
    if (new_tree->type == VNODE_TEXT && strcmp(old_tree->text, new_tree->text) != 0) {
        vdom_dom_update_text(old_tree, new_tree->text);
        strncpy(old_tree->text, new_tree->text, sizeof(old_tree->text) - 1);
    }
    vdom_patch_props(old_tree, new_tree, dom_parent);
    vdom_patch_children(old_tree, new_tree, dom_parent);
}

void vdom_patch_children(VNode* old_parent, VNode* new_parent, void* dom_parent) {
    VNodeList old_list, new_list;
    vdom_node_list_init(&old_list, old_parent->child_count);
    vdom_node_list_init(&new_list, new_parent->child_count);

    for (int i = 0; i < old_parent->child_count; i++)
        vdom_node_list_add(&old_list, old_parent->children[i]);
    for (int i = 0; i < new_parent->child_count; i++)
        vdom_node_list_add(&new_list, new_parent->children[i]);

    vdom_keyed_reconciliation(&old_list, &new_list, dom_parent, old_parent);

    vdom_node_list_free(&old_list);
    vdom_node_list_free(&new_list);
}

void vdom_patch_props(VNode* old_node, VNode* new_node, void* dom_parent) {
    (void)dom_parent;
    for (int i = 0; i < old_node->prop_count; i++) {
        bool found = false;
        for (int j = 0; j < new_node->prop_count; j++) {
            if (strcmp(old_node->props[i].key, new_node->props[j].key) == 0) {
                found = true;
                if (strcmp(old_node->props[i].value, new_node->props[j].value) != 0) {
                    strncpy(old_node->props[i].value, new_node->props[j].value,
                            sizeof(old_node->props[i].value) - 1);
                }
                break;
            }
        }
        if (!found) {
            old_node->props[i].key[0] = '\0';
        }
    }
    for (int i = 0; i < new_node->prop_count; i++) {
        bool found = false;
        for (int j = 0; j < old_node->prop_count; j++) {
            if (strcmp(new_node->props[i].key, old_node->props[j].key) == 0) {
                found = true;
                break;
            }
        }
        if (!found && old_node->prop_count < VDOM_MAX_PROPS) {
            memcpy(&old_node->props[old_node->prop_count], &new_node->props[i], sizeof(VProp));
            old_node->prop_count++;
        }
    }
}

void vdom_keyed_reconciliation(VNodeList* old_children, VNodeList* new_children,
                               void* dom_parent, VNode* parent_node) {
    int max_new = new_children->count;
    int max_old = old_children->count;
    int new_idx = 0;

    for (new_idx = 0; new_idx < max_new && new_idx < max_old; new_idx++) {
        VNode* old_child = vdom_node_list_get(old_children, new_idx);
        VNode* new_child = vdom_node_list_get(new_children, new_idx);

        if (old_child->key[0] && new_child->key[0] &&
            strcmp(old_child->key, new_child->key) == 0) {
            vdom_diff(old_child, new_child, dom_parent);
        } else if (old_child->key[0] && new_child->key[0]) {
            int found_idx = vdom_find_child_by_key(parent_node, new_child->key);
            if (found_idx >= 0) {
                VNode* moved = parent_node->children[found_idx];
                vdom_dom_move_node(moved, dom_parent, new_idx);
                vdom_diff(moved, new_child, dom_parent);
            } else {
                vdom_dom_create_element(new_child, dom_parent);
            }
        } else {
            vdom_diff(old_child, new_child, dom_parent);
        }
    }

    for (int i = new_idx; i < max_old; i++) {
        VNode* old_child = vdom_node_list_get(old_children, i);
        vdom_dom_remove_node(old_child);
    }

    for (int i = new_idx; i < max_new; i++) {
        VNode* new_child = vdom_node_list_get(new_children, i);
        vdom_dom_create_element(new_child, dom_parent);
    }
}

int vdom_find_child_by_key(VNode* parent, const char* key) {
    if (!parent || !key) return -1;
    for (int i = 0; i < parent->child_count; i++) {
        if (strcmp(parent->children[i]->key, key) == 0) return i;
    }
    return -1;
}

void vdom_patch_apply(VPatch* patch) {
    if (!patch) return;
    switch (patch->type) {
        case VPATCH_CREATE:
            if (patch->target) vdom_dom_create_element(patch->target, patch->dom_parent);
            break;
        case VPATCH_REMOVE:
            vdom_dom_remove_node(patch->target);
            break;
        case VPATCH_REPLACE:
            vdom_dom_remove_node(patch->target);
            if (patch->replacement)
                vdom_dom_create_element(patch->replacement, patch->dom_parent);
            break;
        case VPATCH_TEXT:
            vdom_dom_update_text(patch->target, patch->new_text);
            break;
        case VPATCH_PROPS:
            if (patch->target)
                vdom_dom_update_props(patch->target, patch->new_props, patch->prop_count);
            break;
        default:
            break;
    }
}

void vdom_patch(VNode* old_tree, VNode* new_tree) {
    if (!old_tree && !new_tree) return;
    if (!old_tree) {
        VPatch patch = { .type = VPATCH_CREATE, .target = new_tree };
        vdom_patch_apply(&patch);
        return;
    }
    if (!new_tree) {
        VPatch patch = { .type = VPATCH_REMOVE, .target = old_tree };
        vdom_patch_apply(&patch);
        return;
    }
    vdom_diff(old_tree, new_tree, NULL);
}

void vdom_batch_begin(void) {
    g_batch.pending = true;
    g_batch.count = 0;
}

void vdom_batch_end(void) {
    vdom_batch_flush();
    g_batch.pending = false;
}

void vdom_batch_schedule(VPatch* patch) {
    if (!g_batch.pending || g_batch.count >= VDOM_BATCH_SIZE) return;
    g_batch.patches[g_batch.count++] = patch;
}

void vdom_batch_flush(void) {
    for (int i = 0; i < g_batch.count; i++) {
        vdom_patch_apply(g_batch.patches[i]);
    }
    g_batch.count = 0;
}

bool vdom_batch_is_pending(void) {
    return g_batch.pending;
}

void vdom_dom_create_element(VNode* node, void* dom_parent) {
    (void)dom_parent;
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) {
        vdom_dom_create_element(node->children[i], node);
    }
}

void vdom_dom_remove_node(VNode* node) {
    (void)node;
}

void vdom_dom_update_text(VNode* node, const char* text) {
    (void)node;
    (void)text;
}

void vdom_dom_update_props(VNode* node, VProp* props, int count) {
    (void)node;
    (void)props;
    (void)count;
}

void vdom_dom_move_node(VNode* node, void* new_parent, int index) {
    (void)node;
    (void)new_parent;
    (void)index;
}

void vdom_node_list_init(VNodeList* list, int capacity) {
    list->items = (VNode**)calloc(capacity > 0 ? capacity : 16, sizeof(VNode*));
    list->count = 0;
    list->capacity = capacity > 0 ? capacity : 16;
}

void vdom_node_list_add(VNodeList* list, VNode* node) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->items = (VNode**)realloc(list->items, list->capacity * sizeof(VNode*));
    }
    list->items[list->count++] = node;
}

void vdom_node_list_free(VNodeList* list) {
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

VNode* vdom_node_list_get(VNodeList* list, int index) {
    if (index < 0 || index >= list->count) return NULL;
    return list->items[index];
}

void vdom_render_to_dom(VNode* root, void* container) {
    if (!root) return;
    vdom_dom_create_element(root, container);
}

void vdom_update_dom(VNode* old_root, VNode* new_root, void* container) {
    vdom_diff(old_root, new_root, container);
}

const char* vdom_node_type_name(VNodeType type) {
    switch (type) {
        case VNODE_ELEMENT:   return "element";
        case VNODE_TEXT:      return "text";
        case VNODE_FRAGMENT:  return "fragment";
        case VNODE_COMPONENT: return "component";
        default:              return "unknown";
    }
}

void vdom_debug_print(VNode* node, int depth) {
    if (!node) return;
    for (int i = 0; i < depth; i++) printf("  ");
    printf("[%s]", vdom_node_type_name(node->type));
    if (node->type == VNODE_ELEMENT || node->type == VNODE_COMPONENT)
        printf(" <%s>", node->tag);
    if (node->type == VNODE_TEXT)
        printf(" \"%s\"", node->text);
    if (node->key[0])
        printf(" key=%s", node->key);
    printf(" (%d children)", node->child_count);
    printf("\n");
    for (int i = 0; i < node->child_count; i++) {
        vdom_debug_print(node->children[i], depth + 1);
    }
}
