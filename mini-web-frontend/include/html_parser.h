#ifndef HTML_PARSER_H
#define HTML_PARSER_H

#include <stdbool.h>
#include <stddef.h>

#include "dom_tree.h"

typedef enum {
    HTML_TOKEN_NONE,
    HTML_TOKEN_OPEN_TAG,
    HTML_TOKEN_CLOSE_TAG,
    HTML_TOKEN_SELF_CLOSING_TAG,
    HTML_TOKEN_TEXT,
    HTML_TOKEN_COMMENT,
    HTML_TOKEN_DOCTYPE
} HtmlTokenType;

typedef struct {
    char* name;
    char* value;
} HtmlAttribute;

typedef struct {
    HtmlTokenType type;
    char* tag_name;
    HtmlAttribute* attributes;
    size_t attr_count;
    size_t attr_capacity;
    char* text_content;
    bool is_void_element;
} HtmlToken;

typedef struct {
    const char* source;
    size_t source_len;
    size_t pos;
    HtmlToken current_token;
    bool has_error;
    char error_message[256];
} HtmlParser;

void html_parser_init(HtmlParser* parser, const char* html_string);
void html_parser_free(HtmlParser* parser);
bool html_parser_next_token(HtmlParser* parser);
DomNode* html_parse_document(const char* html_string);
DomNode* html_parse_fragment(const char* html_string);

HtmlToken* html_token_create(HtmlTokenType type);
void html_token_free(HtmlToken* token);
void html_token_add_attribute(HtmlToken* token, const char* name, const char* value);

bool html_is_void_element(const char* tag_name);

#endif
