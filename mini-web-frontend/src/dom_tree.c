#include "dom_tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DomNode* dom_node_create(DomNodeType type, const char* tag_name) {
    DomNode* node = (DomNode*)calloc(1, sizeof(DomNode));
    if (!node) return NULL;
    node->type = type;
    if (tag_name) {
        node->tag_name = _strdup(tag_name);
    }
    node->attr_capacity = 4;
    node->attributes = (DomAttribute*)calloc(node->attr_capacity, sizeof(DomAttribute));
    return node;
}

void dom_node_free(DomNode* node) {
    if (!node) return;
    free(node->tag_name);
    free(node->text_content);
    for (size_t i = 0; i < node->attr_count; i++) {
        free(node->attributes[i].name);
        free(node->attributes[i].value);
    }
    free(node->attributes);
    ComputedStyle* cs = node->computed_styles;
    while (cs) {
        ComputedStyle* next = cs->next;
        free(cs->property);
        free(cs->value);
        free(cs);
        cs = next;
    }
    free(node);
}

void dom_node_free_recursive(DomNode* node) {
    if (!node) return;
    DomNode* child = node->first_child;
    while (child) {
        DomNode* next = child->next_sibling;
        dom_node_free_recursive(child);
        child = next;
    }
    dom_node_free(node);
}

void dom_node_append_child(DomNode* parent, DomNode* child) {
    if (!parent || !child) return;
    child->parent = parent;
    if (!parent->first_child) {
        parent->first_child = child;
        parent->last_child = child;
    } else {
        parent->last_child->next_sibling = child;
        child->prev_sibling = parent->last_child;
        parent->last_child = child;
    }
    parent->child_count++;
}

void dom_node_insert_before(DomNode* parent, DomNode* new_node, DomNode* ref_node) {
    if (!parent || !new_node || !ref_node) return;
    new_node->parent = parent;
    new_node->prev_sibling = ref_node->prev_sibling;
    new_node->next_sibling = ref_node;
    if (ref_node->prev_sibling) {
        ref_node->prev_sibling->next_sibling = new_node;
    } else {
        parent->first_child = new_node;
    }
    ref_node->prev_sibling = new_node;
    parent->child_count++;
}

void dom_node_remove_child(DomNode* parent, DomNode* child) {
    if (!parent || !child || child->parent != parent) return;
    if (child->prev_sibling) {
        child->prev_sibling->next_sibling = child->next_sibling;
    } else {
        parent->first_child = child->next_sibling;
    }
    if (child->next_sibling) {
        child->next_sibling->prev_sibling = child->prev_sibling;
    } else {
        parent->last_child = child->prev_sibling;
    }
    child->parent = NULL;
    child->prev_sibling = NULL;
    child->next_sibling = NULL;
    parent->child_count--;
}

DomNode* dom_node_clone(const DomNode* node, bool deep) {
    if (!node) return NULL;
    DomNode* clone = dom_node_create(node->type, node->tag_name);
    if (node->text_content) {
        clone->text_content = _strdup(node->text_content);
    }
    for (size_t i = 0; i < node->attr_count; i++) {
        dom_node_set_attribute(clone, node->attributes[i].name, node->attributes[i].value);
    }
    if (deep) {
        DomNode* child = node->first_child;
        while (child) {
            DomNode* child_clone = dom_node_clone(child, true);
            dom_node_append_child(clone, child_clone);
            child = child->next_sibling;
        }
    }
    return clone;
}

void dom_node_set_attribute(DomNode* node, const char* name, const char* value) {
    if (!node || !name || !value) return;
    for (size_t i = 0; i < node->attr_count; i++) {
        if (_stricmp(node->attributes[i].name, name) == 0) {
            free(node->attributes[i].value);
            node->attributes[i].value = _strdup(value);
            return;
        }
    }
    if (node->attr_count >= node->attr_capacity) {
        node->attr_capacity *= 2;
        node->attributes = (DomAttribute*)realloc(node->attributes, node->attr_capacity * sizeof(DomAttribute));
    }
    node->attributes[node->attr_count].name = _strdup(name);
    node->attributes[node->attr_count].value = _strdup(value);
    node->attr_count++;
}

const char* dom_node_get_attribute(const DomNode* node, const char* name) {
    if (!node || !name) return NULL;
    for (size_t i = 0; i < node->attr_count; i++) {
        if (_stricmp(node->attributes[i].name, name) == 0) {
            return node->attributes[i].value;
        }
    }
    return NULL;
}

bool dom_node_has_attribute(const DomNode* node, const char* name) {
    return dom_node_get_attribute(node, name) != NULL;
}

void dom_node_remove_attribute(DomNode* node, const char* name) {
    if (!node || !name) return;
    for (size_t i = 0; i < node->attr_count; i++) {
        if (_stricmp(node->attributes[i].name, name) == 0) {
            free(node->attributes[i].name);
            free(node->attributes[i].value);
            for (size_t j = i; j < node->attr_count - 1; j++) {
                node->attributes[j] = node->attributes[j + 1];
            }
            node->attr_count--;
            return;
        }
    }
}

void dom_node_set_text(DomNode* node, const char* text) {
    if (!node) return;
    free(node->text_content);
    node->text_content = text ? _strdup(text) : NULL;
}

DomNode* dom_get_element_by_id(const DomNode* root, const char* id) {
    if (!root || !id) return NULL;
    if (root->type == DOM_NODE_ELEMENT) {
        const char* node_id = dom_node_get_attribute(root, "id");
        if (node_id && strcmp(node_id, id) == 0) {
            return (DomNode*)root;
        }
    }
    DomNode* child = root->first_child;
    while (child) {
        DomNode* found = dom_get_element_by_id(child, id);
        if (found) return found;
        child = child->next_sibling;
    }
    return NULL;
}

static void collect_by_tag(const DomNode* root, const char* tag_name, DomNode*** results, size_t* count, size_t* cap) {
    if (!root) return;
    if (root->type == DOM_NODE_ELEMENT && root->tag_name && _stricmp(root->tag_name, tag_name) == 0) {
        if (*count >= *cap) {
            *cap = *cap == 0 ? 16 : *cap * 2;
            *results = (DomNode**)realloc(*results, *cap * sizeof(DomNode*));
        }
        (*results)[(*count)++] = (DomNode*)root;
    }
    DomNode* child = root->first_child;
    while (child) {
        collect_by_tag(child, tag_name, results, count, cap);
        child = child->next_sibling;
    }
}

DomNode** dom_get_elements_by_tag_name(const DomNode* root, const char* tag_name, size_t* out_count) {
    if (!root || !tag_name || !out_count) return NULL;
    DomNode** results = NULL;
    size_t count = 0, cap = 0;
    collect_by_tag(root, tag_name, &results, &count, &cap);
    *out_count = count;
    return results;
}

static bool node_has_class(const DomNode* node, const char* class_name) {
    const char* classes = dom_node_get_attribute(node, "class");
    if (!classes) return false;
    const char* start = classes;
    size_t name_len = strlen(class_name);
    while (*start) {
        while (*start && isspace((unsigned char)*start)) start++;
        const char* end = start;
        while (*end && !isspace((unsigned char)*end)) end++;
        size_t len = (size_t)(end - start);
        if (len == name_len && _strnicmp(start, class_name, len) == 0) return true;
        start = end;
    }
    return false;
}

static void collect_by_class(const DomNode* root, const char* class_name, DomNode*** results, size_t* count, size_t* cap) {
    if (!root) return;
    if (root->type == DOM_NODE_ELEMENT && node_has_class(root, class_name)) {
        if (*count >= *cap) {
            *cap = *cap == 0 ? 16 : *cap * 2;
            *results = (DomNode**)realloc(*results, *cap * sizeof(DomNode*));
        }
        (*results)[(*count)++] = (DomNode*)root;
    }
    DomNode* child = root->first_child;
    while (child) {
        collect_by_class(child, class_name, results, count, cap);
        child = child->next_sibling;
    }
}

DomNode** dom_get_elements_by_class_name(const DomNode* root, const char* class_name, size_t* out_count) {
    if (!root || !class_name || !out_count) return NULL;
    DomNode** results = NULL;
    size_t count = 0, cap = 0;
    collect_by_class(root, class_name, &results, &count, &cap);
    *out_count = count;
    return results;
}

DomNode* dom_query_selector(const DomNode* root, const char* selector) {
    if (!root || !selector) return NULL;
    if (selector[0] == '#') {
        return dom_get_element_by_id(root, selector + 1);
    }
    if (selector[0] == '.') {
        size_t count = 0;
        DomNode** results = dom_get_elements_by_class_name(root, selector + 1, &count);
        if (count > 0) {
            DomNode* first = results[0];
            free(results);
            return first;
        }
        return NULL;
    }
    size_t count = 0;
    DomNode** results = dom_get_elements_by_tag_name(root, selector, &count);
    if (count > 0) {
        DomNode* first = results[0];
        free(results);
        return first;
    }
    return NULL;
}

DomNode** dom_query_selector_all(const DomNode* root, const char* selector, size_t* out_count) {
    if (!root || !selector || !out_count) return NULL;
    if (selector[0] == '.') {
        return dom_get_elements_by_class_name(root, selector + 1, out_count);
    }
    return dom_get_elements_by_tag_name(root, selector, out_count);
}

void dom_traverse_preorder(DomNode* root, DomNodeVisitor visitor, void* user_data) {
    if (!root) return;
    visitor(root, user_data);
    DomNode* child = root->first_child;
    while (child) {
        dom_traverse_preorder(child, visitor, user_data);
        child = child->next_sibling;
    }
}

void dom_traverse_postorder(DomNode* root, DomNodeVisitor visitor, void* user_data) {
    if (!root) return;
    DomNode* child = root->first_child;
    while (child) {
        dom_traverse_postorder(child, visitor, user_data);
        child = child->next_sibling;
    }
    visitor(root, user_data);
}

void dom_node_set_computed_style(DomNode* node, const char* property, const char* value, bool important) {
    if (!node || !property || !value) return;
    ComputedStyle* existing = node->computed_styles;
    while (existing) {
        if (_stricmp(existing->property, property) == 0) {
            free(existing->value);
            existing->value = _strdup(value);
            existing->important = important;
            return;
        }
        existing = existing->next;
    }
    ComputedStyle* cs = (ComputedStyle*)calloc(1, sizeof(ComputedStyle));
    cs->property = _strdup(property);
    cs->value = _strdup(value);
    cs->important = important;
    cs->next = node->computed_styles;
    node->computed_styles = cs;
}

const char* dom_node_get_computed_style(const DomNode* node, const char* property) {
    if (!node || !property) return NULL;
    ComputedStyle* cs = node->computed_styles;
    while (cs) {
        if (_stricmp(cs->property, property) == 0) return cs->value;
        cs = cs->next;
    }
    return NULL;
}

void dom_node_clear_computed_styles(DomNode* node) {
    if (!node) return;
    ComputedStyle* cs = node->computed_styles;
    while (cs) {
        ComputedStyle* next = cs->next;
        free(cs->property);
        free(cs->value);
        free(cs);
        cs = next;
    }
    node->computed_styles = NULL;
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

void dom_print_tree(const DomNode* root, int indent) {
    if (!root) return;
    print_indent(indent);
    switch (root->type) {
        case DOM_NODE_DOCUMENT:
            printf("[DOCUMENT]\n");
            break;
        case DOM_NODE_ELEMENT: {
            printf("<%s", root->tag_name ? root->tag_name : "(null)");
            for (size_t i = 0; i < root->attr_count; i++) {
                printf(" %s=\"%s\"", root->attributes[i].name, root->attributes[i].value);
            }
            printf(">\n");
            break;
        }
        case DOM_NODE_TEXT:
            if (root->text_content) {
                printf("\"%s\"\n", root->text_content);
            } else {
                printf("[TEXT]\n");
            }
            break;
        case DOM_NODE_COMMENT:
            printf("<!-- %s -->\n", root->text_content ? root->text_content : "");
            break;
    }
    DomNode* child = root->first_child;
    while (child) {
        dom_print_tree(child, indent + 1);
        child = child->next_sibling;
    }
}
