#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/design_tokens.h"
#include "../src/component_library.h"
#include "../src/accessibility.h"
#include "../src/interaction_model.h"
#include "../src/theme_system.h"

static void section(const char *title) {
    printf("\n  -- %s --\n", title);
}

static void print_css_var(const char *name, const char *light, const char *dark) {
    printf("  %-30s light: %-20s dark: %s\n", name, light, dark ? dark : "(same)");
}

static void generate_story_component(
    ui_component_t *comp,
    const char *variant,
    const char *size_label,
    ui_variant_t v,
    ui_size_t s,
    const char **html_out)
{
    static char buf[512];
    ui_component_props_t p = ui_props_default(comp->type);
    p.variant = v;
    p.size = s;
    p.label = "Demo";
    ui_render_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.target = RENDER_TARGET_HTML;
    ctx.indent_level = 4;
    ui_component_render_html(comp, &p, &ctx, buf, sizeof(buf));
    *html_out = buf;
}

int main(void) {
    printf("================================================================\n");
    printf("  MINI-UI-UX-HCI  THEME & STORYBOOK\n");
    printf("  Light/Dark Mode + Component Catalog Demo\n");
    printf("================================================================\n");

    ui_tokens_init();
    ui_component_lib_init();
    ui_theme_init();
    ui_a11y_init();
    ui_interaction_init();

    /* ============ THEME SYSTEM ============ */
    section("THEME SYSTEM INITIALIZED");
    printf("  Registered Themes:\n");
    const char **theme_names;
    int theme_count = ui_theme_list_names(&theme_names);
    for (int i = 0; i < theme_count && i < 10; i++) {
        ui_theme_t *t = ui_theme_get(theme_names[i]);
        printf("    %d. %s [%s] %d variables\n",
               i + 1, theme_names[i],
               t->mode == THEME_MODE_LIGHT ? "LIGHT" :
               t->mode == THEME_MODE_DARK  ? "DARK"  : "CUSTOM",
               t->var_count);
    }

    /* ============ LIGHT THEME CSS ============ */
    section("LIGHT THEME CSS CUSTOM PROPERTIES");
    ui_theme_t *light = ui_theme_get("default-light");
    if (light) {
        printf("  Theme: %s (%d variables)\n", light->name, light->var_count);
        printf("  Sample variables:\n");
        for (int i = 0; i < light->var_count && i < 20; i++) {
            ui_theme_variable_t *v = &light->variables[i];
            if (v->name && v->light_value) {
                printf("    %-35s: %s;\n", v->name, v->light_value);
            }
        }
        if (light->var_count > 20)
            printf("    ... and %d more variables\n", light->var_count - 20);
    }

    /* ============ DARK THEME CSS ============ */
    section("DARK THEME CSS CUSTOM PROPERTIES");
    ui_theme_t *dark = ui_theme_get("default-dark");
    if (dark) {
        printf("  Theme: %s (%d variables)\n", dark->name, dark->var_count);
        printf("  Color comparison (light -> dark):\n");
        const char *compare_vars[] = {
            "--color-bg-primary", "--color-bg-secondary", "--color-bg-tertiary",
            "--color-text-primary", "--color-text-secondary", "--color-text-tertiary",
            "--color-border", "--color-border-hover",
            "--color-primary", "--color-primary-hover",
            "--color-success", "--color-warning", "--color-error",
            "--shadow-sm", "--shadow-md", "--shadow-lg", "--shadow-xl",
        };
        for (int i = 0; i < 17; i++) {
            const char *lv = ui_theme_var_get(light, compare_vars[i], false);
            const char *dv = ui_theme_var_get(dark,  compare_vars[i], true);
            print_css_var(compare_vars[i], lv ? lv : "-", dv ? dv : "-");
        }
    }

    /* ============ FULL CSS GENERATION ============ */
    section("FULL CSS OUTPUT");
    ui_theme_context_t *ctx = ui_theme_context_root();
    ui_theme_css_options_t css_opts;
    memset(&css_opts, 0, sizeof(css_opts));
    css_opts.include_root_selector  = true;
    css_opts.include_dark_mode      = true;
    css_opts.include_color_scheme   = true;
    css_opts.include_transition     = true;
    css_opts.nest_dark_media        = true;
    css_opts.indent_spaces          = 2;

    char full_css[16384];
    int css_len = ui_theme_generate_full_css(ctx, &css_opts, full_css, sizeof(full_css));
    printf("  Generated %d bytes of CSS\n", css_len);
    printf("  First 800 chars:\n");
    for (int i = 0; i < 800 && i < css_len; i++) putchar(full_css[i]);
    printf("\n  ... (truncated)\n");

    /* ============ THEME CONTEXT PROPAGATION ============ */
    section("THEME CONTEXT PROPAGATION");
    printf("  Root context: %s\n",
           ui_theme_context_is_dark(ctx) ? "dark" : "light");
    ui_theme_context_dark(ctx);
    printf("  After dark(): %s\n",
           ui_theme_context_is_dark(ctx) ? "dark" : "light");
    ui_theme_context_light(ctx);
    printf("  After light(): %s\n",
           ui_theme_context_is_dark(ctx) ? "dark" : "light");

    ui_theme_context_t *child = ui_theme_context_create_child(ctx);
    printf("  Child context: %s\n",
           ui_theme_context_is_dark(child) ? "dark" : "light");
    printf("  Effective mode: %s\n",
           ui_theme_context_effective_mode(child) == THEME_MODE_LIGHT ? "LIGHT" :
           ui_theme_context_effective_mode(child) == THEME_MODE_DARK  ? "DARK" : "AUTO");

    ui_theme_context_toggle(ctx);
    printf("  After toggle(root): %s\n",
           ui_theme_context_is_dark(ctx) ? "dark" : "light");
    ui_theme_context_light(ctx);

    /* ============ COMPONENT OVERRIDE THEMING ============ */
    section("COMPONENT-LEVEL THEME OVERRIDE");
    ui_theme_override_t *ov = ui_theme_override_create(".card-special", OVERRIDE_SCOPE_CHILDREN);
    ui_theme_override_add_var(ov, "--card-bg", "#fef3c7", "#78350f");
    ui_theme_override_add_var(ov, "--card-border", "#f59e0b", "#d97706");
    ui_theme_override_add_var(ov, "--btn-bg-primary", "#f59e0b", "#d97706");
    ui_theme_override_apply(ov);
    printf("  Applied overrides to .card-special:\n");
    for (int i = 0; i < ov->override_count; i++) {
        printf("    %s: %s (dark: %s)\n",
               ov->overrides[i].name,
               ov->overrides[i].light_value,
               ov->overrides[i].dark_value ? ov->overrides[i].dark_value : "-");
    }

    /* ============ CUSTOM THEME FROM TEMPLATE ============ */
    section("CUSTOM THEME CREATION");
    ui_custom_theme_template_t tpl;
    memset(&tpl, 0, sizeof(tpl));
    tpl.name = "ocean";
    tpl.description = "Ocean blue custom theme";
    tpl.base_theme_name = "default-light";
    tpl.font_family = "Inter, sans-serif";
    tpl.border_radius_scale = "rounded";
    tpl.spacing_scale = "comfortable";

    static struct { const char *key; const char *light; const char *dark; } overrides[] = {
        {"color-bg-primary", "#f0f9ff", "#0c4a6e"},
        {"color-bg-secondary", "#e0f2fe", "#075985"},
        {"color-primary", "#0284c7", "#38bdf8"},
        {"color-text-primary", "#0c4a6e", "#e0f2fe"},
    };
    tpl.color_overrides = (void*)overrides;
    tpl.override_count = 4;

    ui_theme_t *custom = ui_theme_create_from_template(&tpl);
    if (custom) {
        ui_theme_register(custom);
        printf("  Created theme: %s\n", custom->name);
        printf("  Base: %s\n", tpl.base_theme_name);
        printf("  Overrides: %d colors\n", tpl.override_count);
        printf("  Variables:\n");
        for (int i = 0; i < custom->var_count && i < 8; i++) {
            printf("    %-30s: %s;\n",
                   custom->variables[i].name,
                   custom->variables[i].light_value ? custom->variables[i].light_value : "-");
        }
    }

    /* ============ THEME TRANSITIONS ============ */
    section("THEME TRANSITION ANIMATIONS");
    ui_theme_transition_t tr_fade = ui_theme_transition_create(THEME_TRANSITION_FADE, 300);
    tr_fade.easing = "ease-in-out";
    ui_theme_transition_t tr_circle = ui_theme_transition_create(THEME_TRANSITION_CIRCLE, 500);
    tr_circle.origin_x = 0.5f;
    tr_circle.origin_y = 0.5f;

    printf("  Fade transition (300ms):\n");
    char tr_css[512];
    ui_theme_transition_to_css(&tr_fade, tr_css, sizeof(tr_css));
    printf("  %s", tr_css);

    printf("  Circle reveal transition (500ms):\n");
    printf("  origin: (%.1f%%, %.1f%%)\n",
           tr_circle.origin_x * 100, tr_circle.origin_y * 100);
    printf("  easing: %s\n", tr_circle.easing);

    /* ============ META TAGS ============ */
    section("META TAG GENERATION");
    printf("  %s\n", ui_theme_meta_color_scheme(COLOR_SCHEME_LIGHT_DARK));
    printf("  %s\n", ui_theme_meta_theme_color("#ffffff"));
    printf("  %s\n", ui_theme_meta_theme_color_dark("#0f172a"));

    /* ============ STORYBOOK CATALOG ============ */
    section("STORYBOOK: COMPONENT CATALOG");
    printf("  Generating storybook for all 23 components...\n");

    ui_component_catalog_t *storybook_cat = ui_catalog_create(
        "MiniUI Storybook", "1.0.0");

    for (int i = 0; i < COMPONENT_COUNT; i++) {
        ui_component_t *c = ui_component_get((ui_component_type_t)i);
        if (c) ui_catalog_add_component(storybook_cat, c);
    }
    printf("  Catalog has %d components registered.\n", storybook_cat->component_count);

    printf("\n  Component Categories:\n");
    const char **cats;
    int cat_count = ui_catalog_list_categories(&cats);
    for (int i = 0; i < cat_count && i < COMPONENT_COUNT; i++) {
        if (cats[i]) {
            printf("    [%s] %s\n", cats[i],
                   (i < COMPONENT_COUNT) ? component_names : "?");
        }
    }

    /* ============ STORY DEMOS ============ */
    section("INDIVIDUAL STORIES");

    ui_component_props_t props_primary = ui_props_default(COMPONENT_BUTTON);
    props_primary.label = "Click Me";
    props_primary.variant = VARIANT_PRIMARY;

    ui_component_story_t *story_btn = ui_story_create(
        "Button / Primary / Medium", COMPONENT_BUTTON, props_primary);
    story_btn->description = "A primary button for main CTAs";
    printf("  Story: %s\n", story_btn->name);

    ui_component_props_t props_input = ui_props_default(COMPONENT_INPUT);
    props_input.placeholder = "Enter your email";
    props_input.size = SIZE_LG;
    ui_component_story_t *story_input = ui_story_create(
        "Input / Email / Large", COMPONENT_INPUT, props_input);
    story_input->description = "A large email input field";
    printf("  Story: %s\n", story_input->name);

    ui_component_props_t props_modal = ui_props_default(COMPONENT_MODAL);
    props_modal.label = "Dialog Content";
    ui_component_story_t *story_modal = ui_story_create(
        "Modal / Dialog / Medium", COMPONENT_MODAL, props_modal);
    printf("  Story: %s\n", story_modal->name);

    ui_component_props_t props_table = ui_props_default(COMPONENT_TABLE);
    ui_component_story_t *story_table = ui_story_create(
        "Table / Data / Default", COMPONENT_TABLE, props_table);
    printf("  Story: %s\n", story_table->name);

    /* ============ STORYBOOK HTML ============ */
    section("STORYBOOK HTML OUTPUT");
    ui_render_context_t render_ctx;
    memset(&render_ctx, 0, sizeof(render_ctx));
    render_ctx.target = RENDER_TARGET_HTML;
    render_ctx.indent_level = 0;
    render_ctx.dark_mode = false;

    char full_storybook[131072];
    int sb_len = ui_catalog_render_storybook(storybook_cat, &render_ctx,
                                              full_storybook, sizeof(full_storybook));
    printf("  Full storybook HTML generated: %d characters\n", sb_len);
    printf("  Memory usage: %.1f KB\n", (float)sb_len / 1024.0f);

    /* ============ STORYBOOK FILE OUTPUT ============ */
    section("SAVING STORYBOOK TO FILE");
    FILE *out = fopen("storybook_output.html", "w");
    if (out) {
        fwrite(full_storybook, 1, (size_t)sb_len, out);
        fclose(out);
        printf("  Written to: storybook_output.html (%d bytes)\n", sb_len);
    } else {
        printf("  [NOTE] Could not write storybook_output.html (demo mode)\n");
    }

    /* ============ CLEANUP ============ */
    ui_story_free(story_btn);
    ui_story_free(story_input);
    ui_story_free(story_modal);
    ui_story_free(story_table);
    ui_theme_override_remove(ov);
    ui_theme_override_free(ov);
    ui_theme_context_free(child);
    ui_theme_context_free(ctx);
    ui_catalog_free(storybook_cat);

    ui_theme_shutdown();
    ui_interaction_shutdown();
    ui_a11y_shutdown();
    ui_component_lib_shutdown();
    ui_tokens_shutdown();

    printf("\n================================================================\n");
    printf("  THEME & STORYBOOK DEMO COMPLETE\n");
    printf("  Modules demonstrated:\n");
    printf("    - Theme System: light/dark, CSS generation, context\n");
    printf("    - Theme Provider: CSS vars, data-attr, class, media\n");
    printf("    - Theme Override: component-level theming\n");
    printf("    - Custom Themes: template-based creation\n");
    printf("    - Theme Transition: fade, circle reveal animations\n");
    printf("    - Storybook: 23 components, full HTML catalog\n");
    printf("    - Meta Tags: color-scheme, theme-color\n");
    printf("================================================================\n");
    return 0;
}
