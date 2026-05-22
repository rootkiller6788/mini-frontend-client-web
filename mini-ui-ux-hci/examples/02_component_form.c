#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/design_tokens.h"
#include "../src/component_library.h"

int main(void) {
    ui_tokens_init();
    ui_component_lib_init();

    printf("=== Component Library: Form Example ===\n\n");

    ui_render_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.target = RENDER_TARGET_HTML;
    ctx.indent_level = 2;

    /* Create a login form with multiple components */
    printf("Rendering Login Form...\n\n");

    /* Button variants demo */
    printf("--- Button Variants ---\n");
    ui_component_t *btn = ui_component_get(COMPONENT_BUTTON);
    if (btn) {
        ui_variant_t variants[] = {VARIANT_PRIMARY, VARIANT_SECONDARY, VARIANT_OUTLINE, VARIANT_GHOST, VARIANT_DESTRUCTIVE};
        const char *labels[] = {"Save", "Cancel", "Outline", "Ghost", "Delete"};

        for (int i = 0; i < 5; i++) {
            ui_component_props_t p = ui_props_with_variant(COMPONENT_BUTTON, variants[i]);
            p.label = labels[i];
            p.size = SIZE_MD;

            char html[256];
            ui_component_render_html(btn, &p, &ctx, html, sizeof(html));
            printf("  %s\n", html);
        }
    }

    /* Size variants */
    printf("\n--- Button Sizes ---\n");
    ui_size_t sizes[] = {SIZE_XS, SIZE_SM, SIZE_MD, SIZE_LG, SIZE_XL};
    for (int i = 0; i < 5; i++) {
        ui_component_props_t p = ui_props_with_size(COMPONENT_BUTTON, sizes[i]);
        p.label  = "Button";
        char html[256];
        ui_component_render_html(btn, &p, &ctx, html, sizeof(html));
        printf("  %s\n", html);
    }

    /* Input component */
    printf("\n--- Input Fields ---\n");
    ui_component_t *input = ui_component_get(COMPONENT_INPUT);
    if (input) {
        ui_component_props_t p = ui_props_default(COMPONENT_INPUT);
        p.placeholder = "Enter your email...";
        p.label       = "Email";
        p.size        = SIZE_MD;

        char html[256];
        ui_component_render_html(input, &p, &ctx, html, sizeof(html));
        printf("  %s\n", html);

        /* Disabled input */
        p.placeholder = "Disabled input";
        p.disabled    = true;
        ui_component_render_html(input, &p, &ctx, html, sizeof(html));
        printf("  %s\n", html);
    }

    /* Card component */
    printf("\n--- Card Layout ---\n");
    ui_component_t *card = ui_component_get(COMPONENT_CARD);
    if (card) {
        ui_component_props_t p = ui_props_default(COMPONENT_CARD);
        p.label = "Card Content";
        char html[512];
        ui_component_render_html(card, &p, &ctx, html, sizeof(html));
        printf("  %s\n", html);
    }

    /* Component documentation generation */
    printf("\n--- Component Documentation ---\n");
    char docs[1024];
    for (int i = 0; i < 5; i++) {
        ui_component_t *comp = ui_component_get((ui_component_type_t)i);
        if (comp) {
            ui_component_generate_docs(comp, docs, sizeof(docs));
            printf("%s\n", docs);
        }
    }

    /* Render full storybook */
    printf("\n--- Mini Storybook ---\n");
    ui_component_catalog_t *cat = ui_catalog_create("Demo Catalog", "1.0.0");
    ui_catalog_add_component(cat, ui_component_get(COMPONENT_BUTTON));
    ui_catalog_add_component(cat, ui_component_get(COMPONENT_INPUT));
    ui_catalog_add_component(cat, ui_component_get(COMPONENT_CARD));

    char storybook[65536];
    int sb_len = ui_catalog_render_storybook(cat, &ctx, storybook, sizeof(storybook));
    printf("Storybook HTML generated: %d bytes\n", sb_len);

    /* Create stories */
    printf("\n--- Component Stories ---\n");
    ui_component_props_t primary_props  = ui_props_with_variant(COMPONENT_BUTTON, VARIANT_PRIMARY);
    ui_component_props_t secondary_props = ui_props_with_variant(COMPONENT_BUTTON, VARIANT_SECONDARY);
    ui_component_props_t destructive_props = ui_props_with_variant(COMPONENT_BUTTON, VARIANT_DESTRUCTIVE);

    ui_component_story_t *s1 = ui_story_create("Primary Button", COMPONENT_BUTTON, primary_props);
    ui_component_story_t *s2 = ui_story_create("Secondary Button", COMPONENT_BUTTON, secondary_props);
    ui_component_story_t *s3 = ui_story_create("Delete Button", COMPONENT_BUTTON, destructive_props);

    printf("  Story: %s\n", s1->name);
    printf("  Story: %s\n", s2->name);
    printf("  Story: %s\n", s3->name);

    /* State classes */
    printf("\n--- Component State Classes ---\n");
    if (btn) {
        for (int st = STATE_DEFAULT; st <= STATE_LOADING; st++) {
            printf("  %s\n", ui_component_class(btn, (ui_component_state_t)st));
        }
    }

    /* CSS generation */
    printf("\n--- Generated CSS (Button + primary + md) ---\n");
    char btn_css[1024];
    ui_component_props_t css_p = ui_props_default(COMPONENT_BUTTON);
    css_p.variant = VARIANT_PRIMARY;
    css_p.size    = SIZE_MD;
    ui_component_render_css(btn, &css_p, &ctx, btn_css, sizeof(btn_css));
    printf("%s", btn_css);

    /* Cleanup */
    ui_story_free(s1); ui_story_free(s2); ui_story_free(s3);
    ui_catalog_free(cat);
    ui_component_lib_shutdown();
    ui_tokens_shutdown();

    printf("\n=== Form Example Complete ===\n");
    return 0;
}
