#include <stdio.h>
#include <stdlib.h>

#include "html_parser.h"
#include "css_parser.h"
#include "dom_tree.h"
#include "style_resolver.h"
#include "layout_engine.h"

static const char* page_html =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<body>\n"
    "  <header id=\"top-bar\" style=\"height:60px; background:#333; color:white; display:flex; align-items:center; padding:0 20px;\">\n"
    "    <h1>Layout Engine Demo</h1>\n"
    "  </header>\n"
    "  <main style=\"display:flex; min-height:400px;\">\n"
    "    <nav id=\"sidebar\" style=\"width:200px; background:#f5f5f5; padding:15px;\">\n"
    "      <h2>Sidebar</h2>\n"
    "      <ul>\n"
    "        <li>Link One</li>\n"
    "        <li>Link Two</li>\n"
    "        <li>Link Three</li>\n"
    "      </ul>\n"
    "    </nav>\n"
    "    <section id=\"content-area\" style=\"flex:1; padding:20px;\">\n"
    "      <article class=\"post\" style=\"margin-bottom:20px; padding:15px; border:1px solid #ddd;\">\n"
    "        <h2>First Article</h2>\n"
    "        <p>This is the content of the first article. It demonstrates the block layout model.</p>\n"
    "      </article>\n"
    "      <article class=\"post\" style=\"margin-bottom:20px; padding:15px; border:1px solid #ddd;\">\n"
    "        <h2>Second Article</h2>\n"
    "        <p>This is the second article with some <strong>bold text</strong> inside.</p>\n"
    "        <div class=\"tags\" style=\"display:flex; gap:8px; margin-top:10px;\">\n"
    "          <span class=\"tag\" style=\"padding:4px 8px; background:#e0e0e0; border-radius:4px;\">C</span>\n"
    "          <span class=\"tag\" style=\"padding:4px 8px; background:#e0e0e0; border-radius:4px;\">Layout</span>\n"
    "          <span class=\"tag\" style=\"padding:4px 8px; background:#e0e0e0; border-radius:4px;\">Engine</span>\n"
    "        </div>\n"
    "      </article>\n"
    "    </section>\n"
    "  </main>\n"
    "  <footer style=\"height:40px; background:#eee; display:flex; align-items:center; justify-content:center;\">\n"
    "    <p>Layout Demo Footer</p>\n"
    "  </footer>\n"
    "</body>\n"
    "</html>";

static const char* page_css =
    "* { box-sizing: border-box; margin: 0; }\n"
    "body { font-family: Arial, sans-serif; font-size: 16px; }\n"
    "h1 { font-size: 24px; }\n"
    "h2 { font-size: 18px; margin-bottom: 8px; }\n"
    "p { margin-bottom: 8px; line-height: 1.5; }\n"
    "ul { padding-left: 20px; }\n"
    "li { margin-bottom: 4px; }\n"
    ".post { background: white; }\n";

int main(void) {
    printf("=== mini-web-frontend Layout Engine Demo ===\n\n");

    printf("--- Step 1: Parse HTML & CSS ---\n");
    DomNode* document = html_parse_document(page_html);
    CssStylesheet* sheet = css_parse_stylesheet(page_css);
    printf("Parsed HTML document and %zu CSS rules\n", sheet->rule_count);

    printf("\n--- Step 2: Resolve Styles ---\n");
    style_resolve_document(document, sheet);

    printf("\n--- Step 3: Build Layout Tree ---\n");
    LayoutContext ctx;
    layout_context_init(&ctx, 1024.0f, 768.0f);
    printf("Viewport: %.0fx%.0f\n", ctx.viewport_width, ctx.viewport_height);

    LayoutBox* layout_root = layout_compute(document, &ctx);
    if (!layout_root) {
        printf("ERROR: Failed to build layout tree\n");
        css_stylesheet_free(sheet);
        dom_node_free_recursive(document);
        return 1;
    }

    printf("\n--- Layout Tree ---\n");
    layout_print_tree(layout_root, 0);

    printf("\n--- Box Model Analysis ---\n");

    LayoutBox* body_box = layout_root->first_child;
    while (body_box && body_box->dom_node) {
        if (body_box->dom_node->tag_name && _stricmp(body_box->dom_node->tag_name, "body") == 0) break;
        body_box = body_box->next_sibling;
    }

    if (body_box) {
        printf("Body box: display=%d, position=%d, size=%.0fx%.0f\n",
               body_box->display, body_box->position,
               body_box->width, body_box->height);
        printf("  margin:  T=%.0f R=%.0f B=%.0f L=%.0f\n",
               body_box->box.margin_top, body_box->box.margin_right,
               body_box->box.margin_bottom, body_box->box.margin_left);
        printf("  padding: T=%.0f R=%.0f B=%.0f L=%.0f\n",
               body_box->box.padding_top, body_box->box.padding_right,
               body_box->box.padding_bottom, body_box->box.padding_left);
    }

    printf("\n--- Flex Layout Children of <main> ---\n");
    LayoutBox* main_box = NULL;
    LayoutBox* child = layout_root->first_child;
    while (child) {
        if (child->dom_node && child->dom_node->tag_name) {
            DomNode* main_node = dom_query_selector(child->dom_node, "main");
            if (main_node) {
                LayoutBox* gc = child->first_child;
                while (gc) {
                    if (gc->dom_node == main_node) { main_box = gc; break; }
                    gc = gc->next_sibling;
                }
            }
        }
        child = child->next_sibling;
    }

    if (main_box) {
        printf("Found <main> with display=%d, %zu children\n", main_box->display, main_box->child_count);
        LayoutBox* flex_child = main_box->first_child;
        while (flex_child) {
            const char* tag = flex_child->dom_node && flex_child->dom_node->tag_name
                              ? flex_child->dom_node->tag_name : "?";
            printf("  <%s>: pos=(%.0f,%.0f) size=%.0fx%.0f\n",
                   tag, flex_child->x, flex_child->y,
                   flex_child->width, flex_child->height);
            flex_child = flex_child->next_sibling;
        }
    }

    printf("\n--- Fixed / Absolute Position Test ---\n");
    DomNode* top_bar = dom_get_element_by_id(document, "top-bar");
    if (top_bar) {
        const char* height = dom_node_get_computed_style(top_bar, "height");
        const char* bg = dom_node_get_computed_style(top_bar, "background");
        printf("#top-bar: height=%s, background=%s\n", height ? height : "auto", bg ? bg : "none");
    }

    printf("\n--- Cleanup ---\n");
    layout_context_free(&ctx);
    css_stylesheet_free(sheet);
    dom_node_free_recursive(document);
    printf("Done.\n");

    return 0;
}
