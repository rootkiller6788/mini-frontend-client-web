#include "html_parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static void html_skip_whitespace(HtmlParser* parser) {
    while (parser->pos < parser->source_len && isspace((unsigned char)parser->source[parser->pos])) {
        parser->pos++;
    }
}

static void html_skip_until(HtmlParser* parser, char ch) {
    while (parser->pos < parser->source_len && parser->source[parser->pos] != ch) {
        parser->pos++;
    }
}

static char html_peek(const HtmlParser* parser) {
    if (parser->pos >= parser->source_len) return '\0';
    return parser->source[parser->pos];
}

static char html_advance(HtmlParser* parser) {
    if (parser->pos >= parser->source_len) return '\0';
    return parser->source[parser->pos++];
}

void html_parser_init(HtmlParser* parser, const char* html_string) {
    parser->source = html_string;
    parser->source_len = html_string ? strlen(html_string) : 0;
    parser->pos = 0;
    parser->has_error = false;
    parser->error_message[0] = '\0';
    memset(&parser->current_token, 0, sizeof(HtmlToken));
}

void html_parser_free(HtmlParser* parser) {
    if (parser->current_token.tag_name) {
        free(parser->current_token.tag_name);
        parser->current_token.tag_name = NULL;
    }
    if (parser->current_token.attributes) {
        for (size_t i = 0; i < parser->current_token.attr_count; i++) {
            free(parser->current_token.attributes[i].name);
            free(parser->current_token.attributes[i].value);
        }
        free(parser->current_token.attributes);
        parser->current_token.attributes = NULL;
    }
    if (parser->current_token.text_content) {
        free(parser->current_token.text_content);
        parser->current_token.text_content = NULL;
    }
}

HtmlToken* html_token_create(HtmlTokenType type) {
    HtmlToken* token = (HtmlToken*)calloc(1, sizeof(HtmlToken));
    if (!token) return NULL;
    token->type = type;
    return token;
}

void html_token_free(HtmlToken* token) {
    if (!token) return;
    free(token->tag_name);
    if (token->attributes) {
        for (size_t i = 0; i < token->attr_count; i++) {
            free(token->attributes[i].name);
            free(token->attributes[i].value);
        }
        free(token->attributes);
    }
    free(token->text_content);
    free(token);
}

void html_token_add_attribute(HtmlToken* token, const char* name, const char* value) {
    if (!token || !name || !value) return;
    if (token->attr_count >= token->attr_capacity) {
        size_t new_cap = token->attr_capacity == 0 ? 4 : token->attr_capacity * 2;
        HtmlAttribute* new_attrs = (HtmlAttribute*)realloc(token->attributes, new_cap * sizeof(HtmlAttribute));
        if (!new_attrs) return;
        token->attributes = new_attrs;
        token->attr_capacity = new_cap;
    }
    token->attributes[token->attr_count].name = _strdup(name);
    token->attributes[token->attr_count].value = _strdup(value);
    token->attr_count++;
}

bool html_is_void_element(const char* tag_name) {
    static const char* void_elements[] = {
        "area", "base", "br", "col", "embed", "hr", "img", "input",
        "link", "meta", "param", "source", "track", "wbr", NULL
    };
    if (!tag_name) return false;
    for (int i = 0; void_elements[i] != NULL; i++) {
        if (_stricmp(tag_name, void_elements[i]) == 0) return true;
    }
    return false;
}

static char* html_read_while(HtmlParser* parser, int (*pred)(int)) {
    size_t start = parser->pos;
    while (parser->pos < parser->source_len && pred((unsigned char)parser->source[parser->pos])) {
        parser->pos++;
    }
    size_t len = parser->pos - start;
    char* result = (char*)malloc(len + 1);
    if (!result) return NULL;
    memcpy(result, parser->source + start, len);
    result[len] = '\0';
    return result;
}

static int is_tag_char(int c) {
    return isalnum(c) || c == '-' || c == '_';
}

static void html_parse_tag_name(HtmlParser* parser, HtmlToken* token) {
    token->tag_name = html_read_while(parser, is_tag_char);
    if (token->tag_name) {
        for (char* p = token->tag_name; *p; p++) *p = (char)tolower((unsigned char)*p);
    }
}

static void html_parse_attributes(HtmlParser* parser, HtmlToken* token) {
    while (parser->pos < parser->source_len) {
        html_skip_whitespace(parser);
        char c = html_peek(parser);
        if (c == '>' || c == '/' || c == '\0') break;

        char* attr_name = html_read_while(parser, is_tag_char);
        if (!attr_name || *attr_name == '\0') { free(attr_name); break; }

        for (char* p = attr_name; *p; p++) *p = (char)tolower((unsigned char)*p);

        html_skip_whitespace(parser);
        char* attr_value = NULL;

        if (html_peek(parser) == '=') {
            html_advance(parser);
            html_skip_whitespace(parser);
            char quote = html_peek(parser);
            if (quote == '"' || quote == '\'') {
                html_advance(parser);
                size_t val_start = parser->pos;
                html_skip_until(parser, quote);
                size_t val_len = parser->pos - val_start;
                if (html_peek(parser) == quote) html_advance(parser);
                attr_value = (char*)malloc(val_len + 1);
                if (attr_value) {
                    memcpy(attr_value, parser->source + val_start, val_len);
                    attr_value[val_len] = '\0';
                }
            } else {
                attr_value = html_read_while(parser, is_tag_char);
            }
        }

        if (!attr_value) attr_value = _strdup("");

        html_token_add_attribute(token, attr_name, attr_value);
        free(attr_name);
        free(attr_value);
    }
}

bool html_parser_next_token(HtmlParser* parser) {
    if (parser->pos >= parser->source_len) {
        parser->current_token.type = HTML_TOKEN_NONE;
        return false;
    }

    html_parser_free(parser);
    memset(&parser->current_token, 0, sizeof(HtmlToken));

    html_skip_whitespace(parser);
    if (parser->pos >= parser->source_len) {
        parser->current_token.type = HTML_TOKEN_NONE;
        return false;
    }

    char c = html_peek(parser);

    if (c == '<') {
        html_advance(parser);
        c = html_peek(parser);

        if (c == '!') {
            html_advance(parser);
            if (parser->pos + 1 < parser->source_len &&
                parser->source[parser->pos] == '-' && parser->source[parser->pos + 1] == '-') {
                parser->pos += 2;
                parser->current_token.type = HTML_TOKEN_COMMENT;
                size_t start = parser->pos;
                while (parser->pos + 2 < parser->source_len) {
                    if (parser->source[parser->pos] == '-' &&
                        parser->source[parser->pos + 1] == '-' &&
                        parser->source[parser->pos + 2] == '>') {
                        break;
                    }
                    parser->pos++;
                }
                size_t len = parser->pos - start;
                parser->current_token.text_content = (char*)malloc(len + 1);
                if (parser->current_token.text_content) {
                    memcpy(parser->current_token.text_content, parser->source + start, len);
                    parser->current_token.text_content[len] = '\0';
                }
                if (parser->pos + 2 < parser->source_len) parser->pos += 3;
                return true;
            }
            if (_strnicmp(parser->source + parser->pos, "doctype", 7) == 0) {
                parser->current_token.type = HTML_TOKEN_DOCTYPE;
                html_skip_until(parser, '>');
                if (html_peek(parser) == '>') html_advance(parser);
                return true;
            }
        }

        if (c == '/') {
            html_advance(parser);
            parser->current_token.type = HTML_TOKEN_CLOSE_TAG;
            html_parse_tag_name(parser, &parser->current_token);
            html_skip_until(parser, '>');
            if (html_peek(parser) == '>') html_advance(parser);
            return true;
        }

        parser->current_token.type = HTML_TOKEN_OPEN_TAG;
        html_parse_tag_name(parser, &parser->current_token);
        html_parse_attributes(parser, &parser->current_token);

        html_skip_whitespace(parser);
        if (html_peek(parser) == '/') {
            html_advance(parser);
            parser->current_token.type = HTML_TOKEN_SELF_CLOSING_TAG;
        }
        if (html_peek(parser) == '>') html_advance(parser);

        if (parser->current_token.tag_name &&
            html_is_void_element(parser->current_token.tag_name)) {
            parser->current_token.is_void_element = true;
            parser->current_token.type = HTML_TOKEN_SELF_CLOSING_TAG;
        }
        return true;
    }

    parser->current_token.type = HTML_TOKEN_TEXT;
    size_t text_start = parser->pos;
    while (parser->pos < parser->source_len && parser->source[parser->pos] != '<') {
        parser->pos++;
    }
    size_t text_len = parser->pos - text_start;
    if (text_len > 0) {
        parser->current_token.text_content = (char*)malloc(text_len + 1);
        if (parser->current_token.text_content) {
            memcpy(parser->current_token.text_content, parser->source + text_start, text_len);
            parser->current_token.text_content[text_len] = '\0';
        }
    } else {
        parser->current_token.text_content = _strdup("");
    }
    return true;
}

DomNode* html_parse_document(const char* html_string) {
    if (!html_string) return NULL;

    HtmlParser parser;
    html_parser_init(&parser, html_string);

    DomNode* document = dom_node_create(DOM_NODE_DOCUMENT, "#document");
    DomNode* current = document;

    while (html_parser_next_token(&parser)) {
        HtmlToken* tok = &parser.current_token;

        switch (tok->type) {
            case HTML_TOKEN_OPEN_TAG: {
                DomNode* element = dom_node_create(DOM_NODE_ELEMENT, tok->tag_name);
                for (size_t i = 0; i < tok->attr_count; i++) {
                    dom_node_set_attribute(element, tok->attributes[i].name, tok->attributes[i].value);
                }
                dom_node_append_child(current, element);
                if (!tok->is_void_element) {
                    current = element;
                }
                break;
            }
            case HTML_TOKEN_SELF_CLOSING_TAG: {
                DomNode* element = dom_node_create(DOM_NODE_ELEMENT, tok->tag_name);
                for (size_t i = 0; i < tok->attr_count; i++) {
                    dom_node_set_attribute(element, tok->attributes[i].name, tok->attributes[i].value);
                }
                dom_node_append_child(current, element);
                break;
            }
            case HTML_TOKEN_CLOSE_TAG: {
                if (current->parent && current->parent != document) {
                    current = current->parent;
                }
                break;
            }
            case HTML_TOKEN_TEXT: {
                if (tok->text_content && tok->text_content[0] != '\0') {
                    DomNode* text = dom_node_create(DOM_NODE_TEXT, NULL);
                    dom_node_set_text(text, tok->text_content);
                    dom_node_append_child(current, text);
                }
                break;
            }
            case HTML_TOKEN_COMMENT: {
                DomNode* comment = dom_node_create(DOM_NODE_COMMENT, NULL);
                dom_node_set_text(comment, tok->text_content);
                dom_node_append_child(current, comment);
                break;
            }
            default:
                break;
        }
    }

    html_parser_free(&parser);
    return document;
}

DomNode* html_parse_fragment(const char* html_string) {
    DomNode* fragment = dom_node_create(DOM_NODE_ELEMENT, "#fragment");
    if (!html_string) return fragment;

    HtmlParser parser;
    html_parser_init(&parser, html_string);

    while (html_parser_next_token(&parser)) {
        HtmlToken* tok = &parser.current_token;

        switch (tok->type) {
            case HTML_TOKEN_OPEN_TAG:
            case HTML_TOKEN_SELF_CLOSING_TAG: {
                DomNode* element = dom_node_create(DOM_NODE_ELEMENT, tok->tag_name);
                for (size_t i = 0; i < tok->attr_count; i++) {
                    dom_node_set_attribute(element, tok->attributes[i].name, tok->attributes[i].value);
                }
                dom_node_append_child(fragment, element);
                break;
            }
            case HTML_TOKEN_TEXT: {
                if (tok->text_content && tok->text_content[0] != '\0') {
                    DomNode* text = dom_node_create(DOM_NODE_TEXT, NULL);
                    dom_node_set_text(text, tok->text_content);
                    dom_node_append_child(fragment, text);
                }
                break;
            }
            default:
                break;
        }
    }

    html_parser_free(&parser);
    return fragment;
}
