#include <stdio.h>
#include <stdlib.h>

#include "html_parser.h"
#include "css_parser.h"
#include "dom_tree.h"
#include "style_resolver.h"

static const char* sample_html =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head><title>Demo Page</title></head>\n"
    "<body>\n"
    "  <div id=\"header\" class=\"container\">\n"
    "    <h1 class=\"title\">Welcome to mini-web</h1>\n"
    "    <p>This is a <strong>demo</strong> paragraph.</p>\n"
    "  </div>\n"
    "  <div id=\"content\">\n"
    "    <ul class=\"nav\">\n"
    "      <li><a href=\"#home\">Home</a></li>\n"
    "      <li><a href=\"#about\">About</a></li>\n"
    "    </ul>\n"
    "  </div>\n"
    "  <!-- footer comment -->\n"
    "</body>\n"
    "</html>";

static const char* sample_css =
    "body { background: #f0f0f0; font-family: sans-serif; }\n"
    "#header { margin: 20px; padding: 10px; background: #333; }\n"
    ".container { border: 1px solid #ccc; }\n"
    "h1 { color: white; font-size: 24px; }\n"
    "p { color: #666; line-height: 1.5; }\n"
    ".nav { display: flex; padding: 0; }\n"
    ".nav li { list-style: none; margin-right: 10px; }\n"
    "a { color: blue !important; text-decoration: none; }\n"
    "strong { font-weight: bold; }\n";

int main(void) {
    printf("=== mini-web-frontend Parse Demo ===\n\n");

    printf("--- Step 1: Parse HTML ---\n");
    DomNode* document = html_parse_document(sample_html);
    if (!document) {
        printf("ERROR: Failed to parse HTML\n");
        return 1;
    }
    printf("DOM tree built successfully.\n\n");

    printf("--- Step 2: Parse CSS ---\n");
    CssStylesheet* sheet = css_parse_stylesheet(sample_css);
    if (!sheet) {
        printf("ERROR: Failed to parse CSS\n");
        dom_node_free_recursive(document);
        return 1;
    }
    printf("Parsed %zu CSS rules:\n", sheet->rule_count);
    for (size_t i = 0; i < sheet->rule_count; i++) {
        CssRule* rule = &sheet->rules[i];
        printf("  Rule %zu: specificity=(%u,%u,%u) with %zu declarations\n",
               i + 1,
               rule->selector->specificity_a,
               rule->selector->specificity_b,
               rule->selector->specificity_c,
               rule->decl_count);
        for (size_t j = 0; j < rule->decl_count; j++) {
            printf("    %s: %s%s\n",
                   rule->declarations[j].property,
                   rule->declarations[j].value,
                   rule->declarations[j].important ? " !important" : "");
        }
    }

    printf("\n--- Step 3: Resolve Styles ---\n");
    style_resolve_document(document, sheet);
    printf("Style resolution complete.\n");
    style_resolver_print_computed(document);

    printf("\n--- Step 4: DOM Queries ---\n");

    DomNode* header_div = dom_get_element_by_id(document, "header");
    if (header_div) {
        printf("getElementById('header') found: <%s class=\"%s\">\n",
               header_div->tag_name,
               dom_node_get_attribute(header_div, "class"));
    }

    size_t li_count = 0;
    DomNode** li_elems = dom_get_elements_by_tag_name(document, "li", &li_count);
    printf("getElementsByTagName('li') found %zu elements\n", li_count);
    for (size_t i = 0; i < li_count; i++) {
        printf("  [%zu] <%s>\n", i, li_elems[i]->tag_name);
    }
    free(li_elems);

    size_t nav_count = 0;
    DomNode** nav_elems = dom_get_elements_by_class_name(document, "nav", &nav_count);
    printf("getElementsByClassName('nav') found %zu elements\n", nav_count);
    free(nav_elems);

    DomNode* first_a = dom_query_selector(document, "a");
    if (first_a) {
        printf("querySelector('a') found: <%s href=\"%s\">\n",
               first_a->tag_name,
               dom_node_get_attribute(first_a, "href"));
    }

    printf("\n--- Done ---\n");

    css_stylesheet_free(sheet);
    dom_node_free_recursive(document);
    return 0;
}
