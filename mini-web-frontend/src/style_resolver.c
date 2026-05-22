#include "style_resolver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void style_map_init(StyleMap* map) {
    if (!map) return;
    map->head = NULL;
    map->count = 0;
}

void style_map_free(StyleMap* map) {
    if (!map) return;
    StyleProperty* prop = map->head;
    while (prop) {
        StyleProperty* next = prop->next;
        free(prop->name);
        free(prop->value);
        free(prop);
        prop = next;
    }
    map->head = NULL;
    map->count = 0;
}

void style_map_set(StyleMap* map, const char* property, const char* value,
                   StyleSource source, unsigned int specificity) {
    if (!map || !property || !value) return;

    StyleProperty* existing = map->head;
    StyleProperty* prev = NULL;
    while (existing) {
        if (_stricmp(existing->name, property) == 0) {
            bool replace = false;
            if (source == STYLE_SOURCE_AUTHOR_IMPORTANT || source == STYLE_SOURCE_INLINE_IMPORTANT) {
                replace = true;
            } else if (existing->source == STYLE_SOURCE_AUTHOR_IMPORTANT ||
                       existing->source == STYLE_SOURCE_INLINE_IMPORTANT) {
                replace = false;
            } else if (source == STYLE_SOURCE_INLINE) {
                replace = true;
            } else if (existing->source == STYLE_SOURCE_INLINE) {
                replace = false;
            } else if (specificity > existing->specificity) {
                replace = true;
            } else if (specificity == existing->specificity) {
                replace = true;
            }

            if (replace) {
                free(existing->value);
                existing->value = _strdup(value);
                existing->source = source;
                existing->specificity = specificity;
            }
            return;
        }
        prev = existing;
        existing = existing->next;
    }

    StyleProperty* new_prop = (StyleProperty*)calloc(1, sizeof(StyleProperty));
    new_prop->name = _strdup(property);
    new_prop->value = _strdup(value);
    new_prop->source = source;
    new_prop->specificity = specificity;

    if (prev) {
        prev->next = new_prop;
    } else {
        map->head = new_prop;
    }
    map->count++;
}

const char* style_map_get(const StyleMap* map, const char* property) {
    if (!map || !property) return NULL;
    StyleProperty* prop = map->head;
    while (prop) {
        if (_stricmp(prop->name, property) == 0) return prop->value;
        prop = prop->next;
    }
    return NULL;
}

bool style_map_has(const StyleMap* map, const char* property) {
    return style_map_get(map, property) != NULL;
}

void style_resolver_init(StyleResolver* resolver, DomNode* target, CssStylesheet* sheet) {
    if (!resolver) return;
    resolver->computed_map = (StyleMap*)calloc(1, sizeof(StyleMap));
    style_map_init(resolver->computed_map);
    resolver->target_node = target;
    resolver->stylesheet = sheet;
    resolver->resolved = false;
}

void style_resolver_free(StyleResolver* resolver) {
    if (!resolver) return;
    style_map_free(resolver->computed_map);
    free(resolver->computed_map);
    resolver->computed_map = NULL;
}

bool style_match_selector(const DomNode* node, const CssSelector* selector) {
    if (!node || !selector || node->type != DOM_NODE_ELEMENT) return false;

    CssSelectorPart* part = selector->parts;
    while (part) {
        switch (part->type) {
            case CSS_SELECTOR_TAG:
                if (!node->tag_name || _stricmp(node->tag_name, part->value) != 0) return false;
                break;
            case CSS_SELECTOR_ID: {
                const char* id = dom_node_get_attribute(node, "id");
                if (!id || strcmp(id, part->value) != 0) return false;
                break;
            }
            case CSS_SELECTOR_CLASS: {
                const char* classes = dom_node_get_attribute(node, "class");
                if (!classes) return false;
                bool found = false;
                const char* start = classes;
                size_t name_len = strlen(part->value);
                while (*start && !found) {
                    while (*start && isspace((unsigned char)*start)) start++;
                    const char* end = start;
                    while (*end && !isspace((unsigned char)*end)) end++;
                    size_t len = (size_t)(end - start);
                    if (len == name_len && _strnicmp(start, part->value, len) == 0) found = true;
                    start = end;
                }
                if (!found) return false;
                break;
            }
            case CSS_SELECTOR_UNIVERSAL:
                break;
            case CSS_SELECTOR_ATTRIBUTE: {
                const char* attr_name = part->value;
                if (!dom_node_has_attribute(node, attr_name)) return false;
                break;
            }
            default:
                break;
        }
        part = part->next;
    }

    return true;
}

void style_resolve_apply_rule(const DomNode* node, const CssRule* rule, StyleMap* map) {
    if (!node || !rule || !map) return;
    if (!style_match_selector(node, rule->selector)) return;

    unsigned int specificity = rule->selector->specificity_a * 10000 +
                               rule->selector->specificity_b * 100 +
                               rule->selector->specificity_c;

    for (size_t i = 0; i < rule->decl_count; i++) {
        CssDeclaration* decl = &rule->declarations[i];
        StyleSource source = decl->important ? STYLE_SOURCE_AUTHOR_IMPORTANT : STYLE_SOURCE_AUTHOR;
        style_map_set(map, decl->property, decl->value, source, specificity);
    }
}

void style_inherit_properties(const StyleMap* parent, StyleMap* child) {
    if (!parent || !child) return;

    static const char* inheritable[] = {
        "color", "font-size", "font-family", "font-weight", "font-style",
        "line-height", "text-align", "visibility", "white-space",
        "letter-spacing", "word-spacing", "text-indent", "direction", NULL
    };

    for (int i = 0; inheritable[i] != NULL; i++) {
        const char* val = style_map_get(parent, inheritable[i]);
        if (val && !style_map_has(child, inheritable[i])) {
            style_map_set(child, inheritable[i], val, STYLE_SOURCE_USER_AGENT, 0);
        }
    }
}

void style_compute_defaults(StyleMap* map, const char* tag_name) {
    if (!map) return;

    style_map_set(map, "display", "block", STYLE_SOURCE_USER_AGENT, 0);
    style_map_set(map, "position", "static", STYLE_SOURCE_USER_AGENT, 0);
    style_map_set(map, "color", "#000000", STYLE_SOURCE_USER_AGENT, 0);
    style_map_set(map, "font-size", "16px", STYLE_SOURCE_USER_AGENT, 0);
    style_map_set(map, "margin", "0", STYLE_SOURCE_USER_AGENT, 0);
    style_map_set(map, "padding", "0", STYLE_SOURCE_USER_AGENT, 0);
    style_map_set(map, "border", "0", STYLE_SOURCE_USER_AGENT, 0);
    style_map_set(map, "width", "auto", STYLE_SOURCE_USER_AGENT, 0);
    style_map_set(map, "height", "auto", STYLE_SOURCE_USER_AGENT, 0);

    if (!tag_name) return;

    if (_stricmp(tag_name, "body") == 0) {
        style_map_set(map, "margin", "8px", STYLE_SOURCE_USER_AGENT, 0);
    } else if (_stricmp(tag_name, "p") == 0) {
        style_map_set(map, "margin-top", "16px", STYLE_SOURCE_USER_AGENT, 0);
        style_map_set(map, "margin-bottom", "16px", STYLE_SOURCE_USER_AGENT, 0);
    } else if (_stricmp(tag_name, "h1") == 0) {
        style_map_set(map, "font-size", "32px", STYLE_SOURCE_USER_AGENT, 0);
        style_map_set(map, "font-weight", "bold", STYLE_SOURCE_USER_AGENT, 0);
        style_map_set(map, "margin-top", "21px", STYLE_SOURCE_USER_AGENT, 0);
        style_map_set(map, "margin-bottom", "21px", STYLE_SOURCE_USER_AGENT, 0);
    } else if (_stricmp(tag_name, "h2") == 0) {
        style_map_set(map, "font-size", "24px", STYLE_SOURCE_USER_AGENT, 0);
        style_map_set(map, "font-weight", "bold", STYLE_SOURCE_USER_AGENT, 0);
    } else if (_stricmp(tag_name, "span") == 0 || _stricmp(tag_name, "a") == 0 ||
               _stricmp(tag_name, "em") == 0 || _stricmp(tag_name, "strong") == 0) {
        style_map_set(map, "display", "inline", STYLE_SOURCE_USER_AGENT, 0);
    } else if (_stricmp(tag_name, "div") == 0 || _stricmp(tag_name, "section") == 0 ||
               _stricmp(tag_name, "article") == 0 || _stricmp(tag_name, "header") == 0 ||
               _stricmp(tag_name, "footer") == 0 || _stricmp(tag_name, "nav") == 0 ||
               _stricmp(tag_name, "main") == 0) {
        style_map_set(map, "display", "block", STYLE_SOURCE_USER_AGENT, 0);
    } else if (_stricmp(tag_name, "img") == 0) {
        style_map_set(map, "display", "inline-block", STYLE_SOURCE_USER_AGENT, 0);
    }
}

void style_resolve_node(DomNode* node, CssStylesheet* sheet, StyleMap* parent_map) {
    if (!node || node->type != DOM_NODE_ELEMENT) return;

    StyleMap local_map;
    style_map_init(&local_map);
    style_compute_defaults(&local_map, node->tag_name);

    if (parent_map) {
        style_inherit_properties(parent_map, &local_map);
    }

    for (size_t i = 0; i < sheet->rule_count; i++) {
        style_resolve_apply_rule(node, &sheet->rules[i], &local_map);
    }

    StyleProperty* prop = local_map.head;
    while (prop) {
        dom_node_set_computed_style(node, prop->name, prop->value, false);
        prop = prop->next;
    }

    DomNode* child = node->first_child;
    while (child) {
        style_resolve_node(child, sheet, &local_map);
        child = child->next_sibling;
    }

    style_map_free(&local_map);
}

void style_resolve_document(DomNode* dom_root, CssStylesheet* sheet) {
    if (!dom_root || !sheet) return;

    if (dom_root->type == DOM_NODE_DOCUMENT) {
        DomNode* child = dom_root->first_child;
        while (child) {
            style_resolve_node(child, sheet, NULL);
            child = child->next_sibling;
        }
    } else {
        style_resolve_node(dom_root, sheet, NULL);
    }
}

StyleProperty* style_find_conflict(const StyleMap* map, const char* property) {
    if (!map || !property) return NULL;
    StyleProperty* prop = map->head;
    while (prop) {
        if (_stricmp(prop->name, property) == 0) return prop;
        prop = prop->next;
    }
    return NULL;
}

void style_resolver_set_inline_style(DomNode* node, const char* property, const char* value, bool important) {
    if (!node || !property || !value) return;
    dom_node_set_computed_style(node, property, value, important);
}

void style_resolver_print_computed(const DomNode* node) {
    if (!node) return;
    if (node->type == DOM_NODE_ELEMENT) {
        printf("Styles for <%s", node->tag_name ? node->tag_name : "(null)");
        const char* id = dom_node_get_attribute(node, "id");
        if (id) printf("#%s", id);
        const char* cls = dom_node_get_attribute(node, "class");
        if (cls) printf(".%s", cls);
        printf(">:\n");

        ComputedStyle* cs = node->computed_styles;
        while (cs) {
            printf("  %s: %s%s\n", cs->property, cs->value, cs->important ? " !important" : "");
            cs = cs->next;
        }
    }
    DomNode* child = node->first_child;
    while (child) {
        style_resolver_print_computed(child);
        child = child->next_sibling;
    }
}
