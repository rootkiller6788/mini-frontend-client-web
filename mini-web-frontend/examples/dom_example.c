#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "html_parser.h"
#include "dom_tree.h"
#include "css_parser.h"
#include "style_resolver.h"

static void print_node_info(DomNode* node, void* user_data) {
    (void)user_data;
    if (node->type == DOM_NODE_ELEMENT) {
        printf("  ELEMENT: <%s", node->tag_name);
        for (size_t i = 0; i < node->attr_count; i++) {
            printf(" %s=\"%s\"", node->attributes[i].name, node->attributes[i].value);
        }
        printf("> children=%zu\n", node->child_count);
    } else if (node->type == DOM_NODE_TEXT && node->text_content) {
        printf("  TEXT: \"%s\"\n", node->text_content);
    } else if (node->type == DOM_NODE_COMMENT) {
        printf("  COMMENT: \"%s\"\n", node->text_content ? node->text_content : "");
    }
}

int main(void) {
    printf("=== mini-web-frontend DOM/CSS Demo ===\n\n");

    const char* html =
        "<div id=\"app\">\n"
        "  <header id=\"main-header\" class=\"header dark\">\n"
        "    <h1>DOM Tree Demo</h1>\n"
        "  </header>\n"
        "  <main class=\"content\">\n"
        "    <section id=\"intro\" class=\"section\">\n"
        "      <p class=\"text\">Hello <span class=\"highlight\">World</span>!</p>\n"
        "    </section>\n"
        "    <section id=\"details\">\n"
        "      <ul class=\"list\">\n"
        "        <li class=\"item\">Item A</li>\n"
        "        <li class=\"item\">Item B</li>\n"
        "        <li class=\"item active\">Item C</li>\n"
        "      </ul>\n"
        "    </section>\n"
        "  </main>\n"
        "  <footer><p>Footer text</p></footer>\n"
        "</div>";

    const char* css =
        "body { margin: 0; padding: 0; }\n"
        "#app { width: 100%; max-width: 960px; }\n"
        ".header { background: #222; color: white; padding: 20px; }\n"
        ".dark { font-weight: bold; }\n"
        ".content { padding: 15px; }\n"
        ".section { margin-bottom: 20px; }\n"
        ".text { font-size: 16px; line-height: 1.6; }\n"
        ".highlight { color: red; background: yellow; }\n"
        ".list { display: flex; flex-direction: column; }\n"
        ".item { padding: 8px; border-bottom: 1px solid #eee; }\n"
        ".active { background: #e0f0ff; font-weight: bold; }\n"
        "h1 { font-size: 28px; margin: 0; }\n"
        "p { margin: 0 0 10px 0; }\n";

    printf("--- Parsing HTML ---\n");
    DomNode* document = html_parse_document(html);
    printf("Document created with %zu root children\n\n", document->child_count);

    printf("--- Parsing CSS ---\n");
    CssStylesheet* sheet = css_parse_stylesheet(css);
    printf("Stylesheet has %zu rules\n\n", sheet->rule_count);

    printf("--- Full DOM Tree ---\n");
    dom_print_tree(document, 0);

    printf("\n--- Preorder Traversal ---\n");
    dom_traverse_preorder(document, print_node_info, NULL);

    printf("\n--- Resolving Styles ---\n");
    style_resolve_document(document, sheet);

    printf("\n--- Query: getElementById('intro') ---\n");
    DomNode* intro = dom_get_element_by_id(document, "intro");
    if (intro) {
        printf("Found: <%s class=\"%s\">\n",
               intro->tag_name,
               dom_node_get_attribute(intro, "class"));
        const char* margin = dom_node_get_computed_style(intro, "margin-bottom");
        if (margin) printf("  computed margin-bottom: %s\n", margin);
    }

    printf("\n--- Query: getElementsByClassName('item') ---\n");
    size_t item_count = 0;
    DomNode** items = dom_get_elements_by_class_name(document, "item", &item_count);
    printf("Found %zu elements with class 'item':\n", item_count);
    for (size_t i = 0; i < item_count; i++) {
        printf("  [%zu] <%s>", i, items[i]->tag_name);
        if (items[i]->first_child && items[i]->first_child->text_content) {
            printf(" text=\"%s\"", items[i]->first_child->text_content);
        }
        const char* padding = dom_node_get_computed_style(items[i], "padding");
        if (padding) printf(" padding=%s", padding);
        printf("\n");
    }
    free(items);

    printf("\n--- Query: querySelector('.active') ---\n");
    DomNode* active = dom_query_selector(document, ".active");
    if (active) {
        printf("Found: <%s", active->tag_name);
        if (active->first_child && active->first_child->text_content) {
            printf(" text=\"%s\"", active->first_child->text_content);
        }
        printf(">\n");
        const char* bg = dom_node_get_computed_style(active, "background");
        if (bg) printf("  computed background: %s\n", bg);
    }

    printf("\n--- DOM Manipulation ---\n");
    DomNode* new_section = dom_node_create(DOM_NODE_ELEMENT, "section");
    dom_node_set_attribute(new_section, "id", "new-section");
    dom_node_set_attribute(new_section, "class", "section");
    DomNode* new_p = dom_node_create(DOM_NODE_ELEMENT, "p");
    dom_node_set_text(new_p, "Dynamically added paragraph!");
    dom_node_append_child(new_section, new_p);

    DomNode* main_elem = dom_get_element_by_id(document, "app");
    if (main_elem) {
        dom_node_append_child(main_elem, new_section);
        printf("Added new <section id=\"new-section\"> to #app\n");
    }

    printf("\n--- Updated DOM Tree (postorder) ---\n");
    dom_print_tree(document, 0);

    printf("\n--- Clone Test ---\n");
    DomNode* cloned_header = dom_node_clone(intro, true);
    if (cloned_header) {
        printf("Cloned node: <%s class=\"%s\">\n",
               cloned_header->tag_name,
               dom_node_get_attribute(cloned_header, "class"));
        dom_node_free_recursive(cloned_header);
    }

    printf("\n--- Cleanup ---\n");
    css_stylesheet_free(sheet);
    dom_node_free_recursive(document);
    printf("Done.\n");

    return 0;
}
