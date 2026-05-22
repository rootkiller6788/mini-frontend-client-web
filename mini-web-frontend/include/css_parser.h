#ifndef CSS_PARSER_H
#define CSS_PARSER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    CSS_SELECTOR_UNIVERSAL,
    CSS_SELECTOR_TAG,
    CSS_SELECTOR_CLASS,
    CSS_SELECTOR_ID,
    CSS_SELECTOR_ATTRIBUTE,
    CSS_SELECTOR_PSEUDO
} CssSelectorType;

typedef enum {
    CSS_COMBINATOR_NONE,
    CSS_COMBINATOR_DESCENDANT,
    CSS_COMBINATOR_CHILD,
    CSS_COMBINATOR_ADJACENT_SIBLING,
    CSS_COMBINATOR_GENERAL_SIBLING
} CssCombinatorType;

typedef struct CssSelectorPart {
    CssSelectorType type;
    char* value;
    struct CssSelectorPart* next;
} CssSelectorPart;

typedef struct CssSelector {
    CssSelectorPart* parts;
    CssCombinatorType combinator;
    struct CssSelector* next;
    unsigned int specificity_a;
    unsigned int specificity_b;
    unsigned int specificity_c;
} CssSelector;

typedef struct CssDeclaration {
    char* property;
    char* value;
    bool important;
} CssDeclaration;

typedef struct CssRule {
    CssSelector* selector;
    CssDeclaration* declarations;
    size_t decl_count;
    size_t decl_capacity;
} CssRule;

typedef struct CssStylesheet {
    CssRule* rules;
    size_t rule_count;
    size_t rule_capacity;
    char* source_url;
} CssStylesheet;

typedef struct {
    const char* source;
    size_t source_len;
    size_t pos;
    bool has_error;
    char error_message[256];
} CssParser;

void css_parser_init(CssParser* parser, const char* css_string);
void css_parser_free(CssParser* parser);

CssStylesheet* css_parse_stylesheet(const char* css_string);
void css_stylesheet_free(CssStylesheet* sheet);

CssSelector* css_selector_create(void);
void css_selector_free(CssSelector* selector);
void css_selector_add_part(CssSelector* selector, CssSelectorType type, const char* value);
void css_selector_calculate_specificity(CssSelector* selector);
unsigned int css_compare_specificity(const CssSelector* a, const CssSelector* b);

CssRule* css_rule_create(void);
void css_rule_free(CssRule* rule);
void css_rule_add_declaration(CssRule* rule, const char* property, const char* value, bool important);

CssDeclaration* css_declaration_create(const char* property, const char* value, bool important);
void css_declaration_free(CssDeclaration* decl);

#endif
