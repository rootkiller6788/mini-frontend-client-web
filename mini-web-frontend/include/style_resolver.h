#ifndef STYLE_RESOLVER_H
#define STYLE_RESOLVER_H

#include <stdbool.h>
#include <stddef.h>

#include "dom_tree.h"
#include "css_parser.h"

typedef enum {
    STYLE_SOURCE_USER_AGENT,
    STYLE_SOURCE_USER,
    STYLE_SOURCE_AUTHOR,
    STYLE_SOURCE_AUTHOR_IMPORTANT,
    STYLE_SOURCE_USER_IMPORTANT,
    STYLE_SOURCE_INLINE,
    STYLE_SOURCE_INLINE_IMPORTANT
} StyleSource;

typedef struct StyleProperty {
    char* name;
    char* value;
    StyleSource source;
    unsigned int specificity;
    struct StyleProperty* next;
} StyleProperty;

typedef struct StyleMap {
    StyleProperty* head;
    size_t count;
} StyleMap;

typedef struct {
    StyleMap* computed_map;
    DomNode* target_node;
    CssStylesheet* stylesheet;
    bool resolved;
} StyleResolver;

void style_map_init(StyleMap* map);
void style_map_free(StyleMap* map);
void style_map_set(StyleMap* map, const char* property, const char* value, StyleSource source, unsigned int specificity);
const char* style_map_get(const StyleMap* map, const char* property);
bool style_map_has(const StyleMap* map, const char* property);

void style_resolver_init(StyleResolver* resolver, DomNode* target, CssStylesheet* sheet);
void style_resolver_free(StyleResolver* resolver);

void style_resolve_document(DomNode* dom_root, CssStylesheet* sheet);
void style_resolve_node(DomNode* node, CssStylesheet* sheet, StyleMap* parent_map);
void style_resolve_apply_rule(const DomNode* node, const CssRule* rule, StyleMap* map);
bool style_match_selector(const DomNode* node, const CssSelector* selector);

void style_inherit_properties(const StyleMap* parent, StyleMap* child);
void style_compute_defaults(StyleMap* map, const char* tag_name);

StyleProperty* style_find_conflict(const StyleMap* map, const char* property);

void style_resolver_set_inline_style(DomNode* node, const char* property, const char* value, bool important);
void style_resolver_print_computed(const DomNode* node);

#endif
