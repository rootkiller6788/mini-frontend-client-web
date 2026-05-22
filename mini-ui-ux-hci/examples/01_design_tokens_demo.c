#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/design_tokens.h"

int main(void) {
    ui_tokens_init();
    printf("=== Design Token System Demo ===\n\n");

    /* Show palette */
    int pal_count;
    const ui_palette_entry_t *pal = ui_palette_all(&pal_count);
    printf("Color Palette (%d entries):\n", pal_count);
    printf("  First 5 colors:\n");
    for (int i = 0; i < 5 && i < pal_count; i++) {
        printf("    %-14s  rgb(%d,%d,%d)\n",
               pal[i].name, pal[i].r, pal[i].g, pal[i].b);
    }

    /* Spacing scale */
    printf("\nSpacing Scale (4px base):\n");
    ui_spacing_t spaces[] = {SPACE_0, SPACE_1, SPACE_4, SPACE_8, SPACE_16, SPACE_32};
    for (int i = 0; i < 6; i++) {
        printf("  SPACE_%-3d  =>  %dpx  (%s)\n",
               (int)spaces[i], ui_spacing_to_px(spaces[i]),
               ui_spacing_to_rem(spaces[i]));
    }

    /* Typography CSS */
    printf("\nTypography Presets:\n");
    ui_typography_token_t typos[] = {
        {"Body",    FONT_SIZE_BASE, FONT_WEIGHT_NORMAL,  LINE_HEIGHT_NORMAL,  LETTER_SPACING_NORMAL,  FONT_FAMILY_SANS,  NULL},
        {"Heading", FONT_SIZE_3XL,  FONT_WEIGHT_BOLD,    LINE_HEIGHT_TIGHT,   LETTER_SPACING_TIGHTER, FONT_FAMILY_DISPLAY,"Inter, sans-serif"},
        {"Code",    FONT_SIZE_SM,   FONT_WEIGHT_NORMAL,  LINE_HEIGHT_NORMAL,  LETTER_SPACING_NORMAL,  FONT_FAMILY_MONO,   NULL},
    };
    for (int i = 0; i < 3; i++) {
        char css[256];
        ui_typography_to_css(&typos[i], css, sizeof(css));
        printf("  %-12s => %s\n", typos[i].name, css);
    }

    /* Shadow tokens */
    printf("\nShadow Tokens:\n");
    ui_shadow_token_t shadows[] = {
        {"shadow-sm", SHADOW_SM,   0, 1, 2,  0, COLOR_BLACK, 0.05f, false},
        {"shadow-md", SHADOW_MD,   0, 4, 6, -1, COLOR_BLACK, 0.10f, false},
        {"shadow-lg", SHADOW_LG,   0,10,15, -3, COLOR_BLACK, 0.10f, false},
        {"inner",     SHADOW_INNER,0, 2, 4,  0, COLOR_BLACK, 0.05f, true},
    };
    for (int i = 0; i < 4; i++) {
        char css[256];
        ui_shadow_to_css(&shadows[i], NULL, css, sizeof(css));
        printf("  %-12s => %s\n", shadows[i].name, css);
    }

    /* Animation tokens */
    printf("\nAnimation Tokens:\n");
    ui_animation_token_t anims[] = {
        {"fade-in",   EASE_IN_OUT, 200, 0, "opacity"},
        {"slide-up",  EASE_SPRING, 300, 50, "transform"},
        {"scale-out", EASE_BOUNCE, 400, 0, "transform"},
    };
    for (int i = 0; i < 3; i++) {
        char css[256];
        ui_animation_to_css(&anims[i], css, sizeof(css));
        printf("  %-12s => %s\n", anims[i].name, css);
    }

    /* Token collection */
    printf("\nLight Theme Tokens (first 5):\n");
    char light_css[2048];
    ui_tokens_to_css(ui_tokens_global_light(), false, light_css, sizeof(light_css));
    printf("%s", light_css);

    printf("\nDark Theme Tokens (first 5):\n");
    char dark_css[2048];
    ui_tokens_to_css(ui_tokens_global_dark(), true, dark_css, sizeof(dark_css));
    printf("%s", dark_css);

    /* Token layers CSS */
    printf("\n--- Component Layer CSS ---\n");
    char comp_css[1024];
    ui_tokens_layer_to_css(TOKEN_LAYER_COMPONENT, false, comp_css, sizeof(comp_css));
    printf("%s", comp_css);

    /* Token inheritance demo */
    printf("\nToken Inheritance Demo:\n");
    ui_design_token_t *btn_bg = ui_token_find("btn-bg-primary");
    if (btn_bg) {
        printf("  Token: %s\n", btn_bg->name);
        printf("  CSS Var: %s\n", btn_bg->css_variable);
        printf("  Value: %s\n", btn_bg->value);
        printf("  Layer: %d (0=Global, 1=Alias, 2=Component)\n", btn_bg->layer);
    }

    /* Custom token with dark override */
    ui_design_token_t *custom = ui_token_create(
        "custom-bg", TOKEN_LAYER_ALIAS, "--custom-bg", "#faebd7");
    ui_token_set_dark_override(custom, "#1a1a2e");
    printf("\nCustom Token with Dark Override:\n");
    printf("  Name: %s | Light: %s | Dark: %s\n",
           custom->name, custom->value, custom->dark_value);

    /* Token inheritance chain */
    ui_token_set_inherits(custom, "color-bg-primary");
    printf("  Inherits from: %s\n",
           custom->inherits_from ? custom->inherits_from : "(none)");

    printf("\n=== Design Token Demo Complete ===\n");
    ui_tokens_shutdown();
    return 0;
}
