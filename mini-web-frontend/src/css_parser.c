#include "css_parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

void css_parser_init(CssParser* parser, const char* css_string) {
    parser->source = css_string;
    parser->source_len = css_string ? strlen(css_string) : 0;
    parser->pos = 0;
    parser->has_error = false;
    parser->error_message[0] = '\0';
}

void css_parser_free(CssParser* parser) {
    (void)parser;
}

static void css_skip_whitespace(CssParser* parser) {
    while (parser->pos < parser->source_len && isspace((unsigned char)parser->source[parser->pos])) {
        parser->pos++;
    }
}

static void css_skip_comments(CssParser* parser) {
    if (parser->pos + 1 < parser->source_len &&
        parser->source[parser->pos] == '/' && parser->source[parser->pos + 1] == '*') {
        parser->pos += 2;
        while (parser->pos + 1 < parser->source_len) {
            if (parser->source[parser->pos] == '*' && parser->source[parser->pos + 1] == '/') {
                parser->pos += 2;
                return;
            }
            parser->pos++;
        }
    }
}

static char css_peek(const CssParser* parser) {
    if (parser->pos >= parser->source_len) return '\0';
    return parser->source[parser->pos];
}

static char css_advance(CssParser* parser) {
    if (parser->pos >= parser->source_len) return '\0';
    return parser->source[parser->pos++];
}

static char* css_read_while(CssParser* parser, int (*pred)(int)) {
    size_t start = parser->pos;
    while (parser->pos < parser->source_len && pred((unsigned char)parser->source[parser->pos])) {
        parser->pos++;
    }
    size_t len = parser->pos - start;
    if (len == 0) return NULL;
    char* result = (char*)malloc(len + 1);
    if (!result) return NULL;
    memcpy(result, parser->source + start, len);
    result[len] = '\0';
    return result;
}

static int is_ident_char(int c) {
    return isalnum(c) || c == '-' || c == '_';
}

static void css_skip_whitespace_and_comments(CssParser* parser) {
    while (parser->pos < parser->source_len) {
        css_skip_whitespace(parser);
        if (css_peek(parser) == '/' && parser->pos + 1 < parser->source_len &&
            parser->source[parser->pos + 1] == '*') {
            css_skip_comments(parser);
        } else {
            break;
        }
    }
}

static char* css_read_until(CssParser* parser, char ch) {
    size_t start = parser->pos;
    while (parser->pos < parser->source_len && parser->source[parser->pos] != ch) {
        parser->pos++;
    }
    size_t len = parser->pos - start;
    char* result = (char*)malloc(len + 1);
    if (!result) return NULL;
    memcpy(result, parser->source + start, len);
    result[len] = '\0';
    return result;
}

static void css_parse_selectors(CssParser* parser, CssRule* rule) {
    css_skip_whitespace_and_comments(parser);

    CssSelector* current_sel = css_selector_create();
    if (!rule->selector) {
        rule->selector = current_sel;
    } else {
        CssSelector* tail = rule->selector;
        while (tail->next) tail = tail->next;
        tail->next = current_sel;
    }

    while (parser->pos < parser->source_len) {
        css_skip_whitespace_and_comments(parser);
        char c = css_peek(parser);

        if (c == '{' || c == ',' || c == '\0') break;

        if (c == '.') {
            css_advance(parser);
            char* name = css_read_while(parser, is_ident_char);
            if (name) {
                css_selector_add_part(current_sel, CSS_SELECTOR_CLASS, name);
                free(name);
            }
        } else if (c == '#') {
            css_advance(parser);
            char* name = css_read_while(parser, is_ident_char);
            if (name) {
                css_selector_add_part(current_sel, CSS_SELECTOR_ID, name);
                free(name);
            }
        } else if (c == '*') {
            css_advance(parser);
            css_selector_add_part(current_sel, CSS_SELECTOR_UNIVERSAL, "*");
        } else if (c == ':') {
            css_advance(parser);
            char* name = css_read_while(parser, is_ident_char);
            if (name) {
                css_selector_add_part(current_sel, CSS_SELECTOR_PSEUDO, name);
                free(name);
            }
        } else if (c == '[') {
            css_advance(parser);
            char* attr = css_read_until(parser, ']');
            if (attr) {
                css_selector_add_part(current_sel, CSS_SELECTOR_ATTRIBUTE, attr);
                free(attr);
            }
            if (css_peek(parser) == ']') css_advance(parser);
        } else if (c == '>') {
            css_advance(parser);
            current_sel->combinator = CSS_COMBINATOR_CHILD;
        } else if (c == '+') {
            css_advance(parser);
            current_sel->combinator = CSS_COMBINATOR_ADJACENT_SIBLING;
        } else if (c == '~') {
            css_advance(parser);
            current_sel->combinator = CSS_COMBINATOR_GENERAL_SIBLING;
        } else if (isalpha((unsigned char)c) || c == '-' || c == '_') {
            char* name = css_read_while(parser, is_ident_char);
            if (name) {
                css_selector_add_part(current_sel, CSS_SELECTOR_TAG, name);
                free(name);
            }
        } else {
            css_advance(parser);
        }
    }

    css_selector_calculate_specificity(current_sel);

    if (css_peek(parser) == ',') {
        css_advance(parser);
        css_parse_selectors(parser, rule);
    }
}

static void css_parse_declaration_block(CssParser* parser, CssRule* rule) {
    if (css_peek(parser) != '{') return;
    css_advance(parser);

    while (parser->pos < parser->source_len) {
        css_skip_whitespace_and_comments(parser);
        if (css_peek(parser) == '}' || css_peek(parser) == '\0') break;

        char* property = css_read_while(parser, is_ident_char);
        if (!property) break;

        css_skip_whitespace_and_comments(parser);
        if (css_peek(parser) == ':') css_advance(parser);
        css_skip_whitespace_and_comments(parser);

        char* value = css_read_until(parser, ';');
        if (!value) value = css_read_until(parser, '}');
        if (!value) { free(property); break; }

        bool important = false;
        size_t vlen = strlen(value);
        while (vlen > 0 && isspace((unsigned char)value[vlen - 1])) { value[--vlen] = '\0'; }

        const char* imp_marker = "!important";
        size_t imp_len = 10;
        if (vlen >= imp_len) {
            char* found = strstr(value, imp_marker);
            if (found) {
                important = true;
                *found = '\0';
                vlen = strlen(value);
                while (vlen > 0 && isspace((unsigned char)value[vlen - 1])) { value[--vlen] = '\0'; }
            }
        }

        if (property[0] && value[0]) {
            css_rule_add_declaration(rule, property, value, important);
        }

        free(property);
        free(value);

        if (css_peek(parser) == ';') css_advance(parser);
    }

    if (css_peek(parser) == '}') css_advance(parser);
}

CssStylesheet* css_parse_stylesheet(const char* css_string) {
    if (!css_string) return NULL;

    CssStylesheet* sheet = (CssStylesheet*)calloc(1, sizeof(CssStylesheet));
    if (!sheet) return NULL;
    sheet->rule_capacity = 8;
    sheet->rules = (CssRule*)calloc(sheet->rule_capacity, sizeof(CssRule));

    CssParser parser;
    css_parser_init(&parser, css_string);

    while (parser.pos < parser.source_len) {
        css_skip_whitespace_and_comments(&parser);
        if (parser.pos >= parser.source_len) break;

        if (css_peek(&parser) == '@') {
            css_skip_until(&parser, ';');
            continue;
        }

        if (sheet->rule_count >= sheet->rule_capacity) {
            sheet->rule_capacity *= 2;
            sheet->rules = (CssRule*)realloc(sheet->rules, sheet->rule_capacity * sizeof(CssRule));
        }

        css_parse_selectors(&parser, &sheet->rules[sheet->rule_count]);
        css_parse_declaration_block(&parser, &sheet->rules[sheet->rule_count]);
        sheet->rule_count++;
    }

    css_parser_free(&parser);
    return sheet;
}

void css_stylesheet_free(CssStylesheet* sheet) {
    if (!sheet) return;
    for (size_t i = 0; i < sheet->rule_count; i++) {
        css_rule_free(&sheet->rules[i]);
    }
    free(sheet->rules);
    free(sheet->source_url);
    free(sheet);
}

CssSelector* css_selector_create(void) {
    CssSelector* sel = (CssSelector*)calloc(1, sizeof(CssSelector));
    return sel;
}

void css_selector_free(CssSelector* selector) {
    if (!selector) return;
    CssSelectorPart* part = selector->parts;
    while (part) {
        CssSelectorPart* next = part->next;
        free(part->value);
        free(part);
        part = next;
    }
    free(selector);
}

void css_selector_add_part(CssSelector* selector, CssSelectorType type, const char* value) {
    if (!selector || !value) return;
    CssSelectorPart* part = (CssSelectorPart*)calloc(1, sizeof(CssSelectorPart));
    part->type = type;
    part->value = _strdup(value);
    if (!selector->parts) {
        selector->parts = part;
    } else {
        CssSelectorPart* tail = selector->parts;
        while (tail->next) tail = tail->next;
        tail->next = part;
    }
}

void css_selector_calculate_specificity(CssSelector* selector) {
    if (!selector) return;
    selector->specificity_a = 0;
    selector->specificity_b = 0;
    selector->specificity_c = 0;

    CssSelectorPart* part = selector->parts;
    while (part) {
        switch (part->type) {
            case CSS_SELECTOR_ID:
                selector->specificity_a++;
                break;
            case CSS_SELECTOR_CLASS:
            case CSS_SELECTOR_ATTRIBUTE:
            case CSS_SELECTOR_PSEUDO:
                selector->specificity_b++;
                break;
            case CSS_SELECTOR_TAG:
                selector->specificity_c++;
                break;
            default:
                break;
        }
        part = part->next;
    }
}

unsigned int css_compare_specificity(const CssSelector* a, const CssSelector* b) {
    if (!a && !b) return 0;
    if (!a) return 1;
    if (!b) return 2;

    if (a->specificity_a != b->specificity_a)
        return a->specificity_a > b->specificity_a ? 2 : 1;
    if (a->specificity_b != b->specificity_b)
        return a->specificity_b > b->specificity_b ? 2 : 1;
    if (a->specificity_c != b->specificity_c)
        return a->specificity_c > b->specificity_c ? 2 : 1;
    return 0;
}

CssRule* css_rule_create(void) {
    return (CssRule*)calloc(1, sizeof(CssRule));
}

void css_rule_free(CssRule* rule) {
    if (!rule) return;
    CssSelector* sel = rule->selector;
    while (sel) {
        CssSelector* next = sel->next;
        css_selector_free(sel);
        sel = next;
    }
    for (size_t i = 0; i < rule->decl_count; i++) {
        css_declaration_free(&rule->declarations[i]);
    }
    free(rule->declarations);
}

void css_rule_add_declaration(CssRule* rule, const char* property, const char* value, bool important) {
    if (!rule || !property || !value) return;
    if (rule->decl_count >= rule->decl_capacity) {
        size_t new_cap = rule->decl_capacity == 0 ? 8 : rule->decl_capacity * 2;
        CssDeclaration* new_decls = (CssDeclaration*)realloc(rule->declarations, new_cap * sizeof(CssDeclaration));
        if (!new_decls) return;
        if (rule->decl_capacity == 0) memset(new_decls, 0, new_cap * sizeof(CssDeclaration));
        rule->declarations = new_decls;
        rule->decl_capacity = new_cap;
    }
    rule->declarations[rule->decl_count].property = _strdup(property);
    rule->declarations[rule->decl_count].value = _strdup(value);
    rule->declarations[rule->decl_count].important = important;
    rule->decl_count++;
}

CssDeclaration* css_declaration_create(const char* property, const char* value, bool important) {
    CssDeclaration* decl = (CssDeclaration*)calloc(1, sizeof(CssDeclaration));
    if (!decl) return NULL;
    decl->property = _strdup(property);
    decl->value = _strdup(value);
    decl->important = important;
    return decl;
}

void css_declaration_free(CssDeclaration* decl) {
    if (!decl) return;
    free(decl->property);
    free(decl->value);
}

static void css_skip_until(CssParser* parser, char ch) {
    while (parser->pos < parser->source_len && parser->source[parser->pos] != ch) {
        parser->pos++;
    }
}
