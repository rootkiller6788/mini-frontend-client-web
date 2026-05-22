#ifndef DOM_TREE_H
#define DOM_TREE_H

#include <stdbool.h>
#include <stddef.h>

#define DOM_MAX_CHILDREN 1024
#define DOM_MAX_DEPTH 256

typedef enum {
    DOM_NODE_ELEMENT,
    DOM_NODE_TEXT,
    DOM_NODE_COMMENT,
    DOM_NODE_DOCUMENT
} DomNodeType;

typedef struct DomAttribute {
    char* name;
    char* value;
} DomAttribute;

typedef struct ComputedStyle {
    char* property;
    char* value;
    bool important;
    struct ComputedStyle* next;
} ComputedStyle;

typedef struct DomNode {
    DomNodeType type;
    char* tag_name;
    char* text_content;
    DomAttribute* attributes;
    size_t attr_count;
    size_t attr_capacity;
    struct DomNode* parent;
    struct DomNode* first_child;
    struct DomNode* last_child;
    struct DomNode* next_sibling;
    struct DomNode* prev_sibling;
    ComputedStyle* computed_styles;
    size_t child_count;
} DomNode;

DomNode* dom_node_create(DomNodeType type, const char* tag_name);
void dom_node_free(DomNode* node);
void dom_node_free_recursive(DomNode* node);

void dom_node_append_child(DomNode* parent, DomNode* child);
void dom_node_insert_before(DomNode* parent, DomNode* new_node, DomNode* ref_node);
void dom_node_remove_child(DomNode* parent, DomNode* child);
DomNode* dom_node_clone(const DomNode* node, bool deep);

void dom_node_set_attribute(DomNode* node, const char* name, const char* value);
const char* dom_node_get_attribute(const DomNode* node, const char* name);
bool dom_node_has_attribute(const DomNode* node, const char* name);
void dom_node_remove_attribute(DomNode* node, const char* name);

void dom_node_set_text(DomNode* node, const char* text);

DomNode* dom_get_element_by_id(const DomNode* root, const char* id);
DomNode** dom_get_elements_by_tag_name(const DomNode* root, const char* tag_name, size_t* out_count);
DomNode** dom_get_elements_by_class_name(const DomNode* root, const char* class_name, size_t* out_count);
DomNode* dom_query_selector(const DomNode* root, const char* selector);
DomNode** dom_query_selector_all(const DomNode* root, const char* selector, size_t* out_count);

typedef void (*DomNodeVisitor)(DomNode* node, void* user_data);
void dom_traverse_preorder(DomNode* root, DomNodeVisitor visitor, void* user_data);
void dom_traverse_postorder(DomNode* root, DomNodeVisitor visitor, void* user_data);

void dom_node_set_computed_style(DomNode* node, const char* property, const char* value, bool important);
const char* dom_node_get_computed_style(const DomNode* node, const char* property);
void dom_node_clear_computed_styles(DomNode* node);

void dom_print_tree(const DomNode* root, int indent);

#endif
