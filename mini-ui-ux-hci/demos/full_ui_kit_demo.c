#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/design_tokens.h"
#include "../src/component_library.h"
#include "../src/accessibility.h"
#include "../src/interaction_model.h"

static void print_separator(const char *title) {
    printf("\n%s\n", "========================================");
    printf("  %s\n", title);
    printf("%s\n\n", "========================================");
}

static void demo_design_tokens(void) {
    print_separator("1. DESIGN TOKENS");

    ui_tokens_init();

    printf("  Token Layers:\n");
    printf("    Global   -> Raw values (e.g. --color-blue-500: #3b82f6)\n");
    printf("    Alias    -> Semantic reference (e.g. --color-bg-primary: var(--color-white))\n");
    printf("    Component-> Scoped tokens (e.g. --btn-bg-primary: var(--color-blue-500))\n");

    printf("\n  Spacing Scale (4px base):\n");
    ui_spacing_t demo_spaces[] = {SPACE_1, SPACE_2, SPACE_4, SPACE_6, SPACE_8, SPACE_12};
    for (int i = 0; i < 6; i++) {
        printf("    %s ", ui_spacing_to_rem(demo_spaces[i]));
        if (i % 3 == 2) printf("\n");
    }

    printf("\n  Border Radius Scale:\n");
    printf("    none(0) sm(2px) base(4px) md(6px) lg(8px) xl(12px) 2xl(16px) 3xl(24px) full(9999px)\n");

    printf("\n  Shadow Levels:\n");
    printf("    sm: 0 1px 2px 0 rgb(0 0 0 / 0.05)\n");
    printf("    md: 0 4px 6px -1px rgb(0 0 0 / 0.1)\n");
    printf("    lg: 0 10px 15px -3px rgb(0 0 0 / 0.1)\n");
    printf("    xl: 0 20px 25px -5px rgb(0 0 0 / 0.1)\n");

    printf("\n  Breakpoints:\n");
    printf("    sm:640px  md:768px  lg:1024px  xl:1280px  2xl:1536px\n");

    printf("\n  Generating CSS Custom Properties:\n");
    char css[4096];
    const ui_token_collection_t *light = ui_tokens_global_light();
    ui_tokens_to_css(light, false, css, sizeof(css));
    printf("%s", css);

    ui_tokens_shutdown();
}

static void demo_component_library(void) {
    print_separator("2. COMPONENT LIBRARY");

    ui_component_lib_init();

    printf("  Registered Components:\n");
    const char **names;
    int count = ui_component_list_names(&names);
    for (int i = 0; i < count; i++) {
        printf("    %2d. %-15s  [%s]  <%s>\n",
               i + 1, names[i],
               ui_component_is_container((ui_component_type_t)i) ? "container" : "element",
               component_tags ? component_tags[i] : "div");
    }

    printf("\n  Button: variant x size matrix:\n");
    ui_component_t *btn = ui_component_get(COMPONENT_BUTTON);
    if (btn) {
        const char *var_names[]  = {"Primary","Secondary","Outline","Ghost","Destructive"};
        const char *size_names[] = {"XS","SM","MD","LG","XL"};
        ui_variant_t vars[]  = {VARIANT_PRIMARY,VARIANT_SECONDARY,VARIANT_OUTLINE,VARIANT_GHOST,VARIANT_DESTRUCTIVE};
        ui_size_t    szs[]   = {SIZE_XS,SIZE_SM,SIZE_MD,SIZE_LG,SIZE_XL};
        printf("    %-12s", "");
        for (int s = 0; s < 5; s++) printf("%-5s ", size_names[s]);
        printf("\n");
        for (int v = 0; v < 5; v++) {
            printf("    %-12s", var_names[v]);
            for (int s = 0; s < 5; s++) {
                ui_component_props_t p = ui_props_default(COMPONENT_BUTTON);
                p.variant = vars[v]; p.size = szs[s];
                char cls[64]; snprintf(cls,sizeof(cls),"%s-%s-%s",btn->name,
                    var_names[v], size_names[s]);
                printf("%-5s ", cls[0]=='B'?"B":"");
            }
            printf("\n");
        }
    }

    printf("\n  Component States (Button):\n");
    if (btn) {
        const char *st_names[] = {"default","hover","active","focus","disabled","loading"};
        for (int s = 0; s < 6; s++) {
            printf("    .%s--%s\n", btn->name, st_names[s]);
        }
    }

    printf("\n  Storybook HTML Demo (first 200 chars):\n");
    ui_component_catalog_t *cat = ui_catalog_create("Demo Kit", "1.0.0");
    for (int i = 0; i < 5; i++)
        ui_catalog_add_component(cat, ui_component_get((ui_component_type_t)i));

    ui_render_context_t ctx;
    memset(&ctx, 0, sizeof(ctx)); ctx.target = RENDER_TARGET_HTML;
    char sb[65536];
    int len = ui_catalog_render_storybook(cat, &ctx, sb, sizeof(sb));
    printf("    Generated %d bytes of storybook HTML\n", len);

    ui_catalog_free(cat);
    ui_component_lib_shutdown();
}

static void demo_accessibility_tree(void) {
    print_separator("3. ACCESSIBILITY TREE & WCAG");

    ui_a11y_init();

    printf("  ARIA Landmark Roles:\n");
    ui_aria_role_t landmarks[] = {ARIA_ROLE_BANNER,ARIA_ROLE_MAIN,ARIA_ROLE_NAVIGATION,ARIA_ROLE_COMPLEMENTARY,ARIA_ROLE_CONTENTINFO};
    for (int i = 0; i < 5; i++) {
        printf("    %s - %s\n", ui_aria_role_name(landmarks[i]),
               ui_aria_is_landmark(landmarks[i]) ? "landmark" : "non-landmark");
    }

    printf("\n  Building Full Page A11y Tree:\n");
    ui_a11y_node_t *root    = ui_a11y_node_create("Document", ARIA_ROLE_DOCUMENT);
    ui_a11y_node_t *main    = ui_a11y_node_create("Main Content", ARIA_ROLE_MAIN);
    ui_a11y_node_t *banner  = ui_a11y_node_create("Header", ARIA_ROLE_BANNER);
    ui_a11y_node_t *nav     = ui_a11y_node_create("Primary Nav", ARIA_ROLE_NAVIGATION);
    ui_a11y_node_t *nav_item1 = ui_a11y_node_create("Home", ARIA_ROLE_LINK);
    ui_a11y_node_t *nav_item2 = ui_a11y_node_create("About", ARIA_ROLE_LINK);
    ui_a11y_node_t *nav_item3 = ui_a11y_node_create("Contact", ARIA_ROLE_LINK);
    ui_a11y_node_t *search   = ui_a11y_node_create("Search", ARIA_ROLE_SEARCHBOX);
    ui_a11y_node_t *h1       = ui_a11y_node_create("Page Title", ARIA_ROLE_HEADING);
    ui_a11y_node_t *article  = ui_a11y_node_create("Article", ARIA_ROLE_ARTICLE);
    ui_a11y_node_t *h2       = ui_a11y_node_create("Section", ARIA_ROLE_HEADING);
    ui_a11y_node_t *region   = ui_a11y_node_create("Sidebar", ARIA_ROLE_REGION);
    ui_a11y_node_t *footer   = ui_a11y_node_create("Footer", ARIA_ROLE_CONTENTINFO);

    ui_a11y_node_add_child(root, banner);
    ui_a11y_node_add_child(banner, nav);
    ui_a11y_node_add_child(nav, nav_item1);
    ui_a11y_node_add_child(nav, nav_item2);
    ui_a11y_node_add_child(nav, nav_item3);
    ui_a11y_node_add_child(root, main);
    ui_a11y_node_add_child(main, h1);
    ui_a11y_node_add_child(main, article);
    ui_a11y_node_add_child(article, h2);
    ui_a11y_node_add_child(main, region);
    ui_a11y_node_add_child(main, search);
    ui_a11y_node_add_child(root, footer);

    char tree[4096];
    ui_a11y_tree_to_text(root, tree, sizeof(tree));
    printf("%s", tree);

    printf("\n  Focus Management Demo:\n");
    ui_focus_manager_t *fm = ui_focus_manager_create();
    int tab = 1;
    ui_a11y_assign_tab_order(root, &tab);
    ui_focus_set(fm, nav_item1);
    for (int i = 0; i < 4; i++) {
        printf("    Focus[%d]: %s (tab:%d)\n", i,
               ui_focus_get_current(fm)->name,
               ui_focus_get_current(fm)->tab_order);
        ui_focus_move(fm, FOCUS_DIRECTION_NEXT);
    }

    printf("\n  Contrast Checker:\n");
    struct { const char *fg; const char *bg; const char *desc; } ct[] = {
        {"#0f172a","#ffffff","Text/Bg"}, {"#64748b","#ffffff","Secondary/Bg"},
        {"#3b82f6","#ffffff","Btn/Bg"},  {"#94a3b8","#ffffff","Muted/Bg"},
        {"#ffffff","#0f172a","White/Black"},{"#ef4444","#ffffff","Error/Bg"},
    };
    for (int i = 0; i < 6; i++) {
        ui_contrast_result_t r = ui_contrast_check_hex(ct[i].fg, ct[i].bg);
        printf("    %-18s ratio:%.2f  AA:%s AAA:%s\n", ct[i].desc, r.ratio,
               r.passes_aa_normal?"PASS":"FAIL", r.passes_aaa_normal?"PASS":"FAIL");
    }

    ui_a11y_node_free(root);
    ui_focus_manager_free(fm);
    ui_a11y_shutdown();
}

static void demo_interaction_model(void) {
    print_separator("4. INTERACTION MODEL (Touch & Gestures)");

    ui_interaction_init();

    printf("  Pointer Event Flow:\n");
    printf("    touchstart -> touchmove -> touchend\n");
    printf("    pointerdown -> pointermove -> pointerup\n");

    printf("\n  Gesture Recognition:\n");
    ui_gesture_recognizer_t *rec = ui_gesture_recognizer_create();

    ui_pointer_event_t down = ui_pointer_event_create(POINTER_EVENT_DOWN, 100, 200, POINTER_TYPE_TOUCH);
    ui_pointer_event_t move_swipe = ui_pointer_event_create(POINTER_EVENT_MOVE, 250, 200, POINTER_TYPE_TOUCH);
    ui_pointer_event_t up_swipe   = ui_pointer_event_create(POINTER_EVENT_UP, 250, 200, POINTER_TYPE_TOUCH);
    ui_pointer_event_t up_tap     = ui_pointer_event_create(POINTER_EVENT_UP, 102, 201, POINTER_TYPE_TOUCH);

    printf("    Feed touchstart...\n");
    ui_gesture_feed_event(rec, &down);
    printf("    Feed swipe-move 150px...\n");
    ui_gesture_type_t g = ui_gesture_feed_event(rec, &move_swipe);
    printf("    Recognized: %s\n",
           g == GESTURE_SWIPE_RIGHT ? "SWIPE_RIGHT" :
           g == GESTURE_SWIPE_LEFT  ? "SWIPE_LEFT"  : "NONE");
    ui_gesture_feed_event(rec, &up_swipe);

    printf("\n    Reset, feed tap...\n");
    ui_gesture_reset(rec);
    ui_gesture_feed_event(rec, &down);
    g = ui_gesture_feed_event(rec, &up_tap);
    printf("    Recognized: %s\n", g == GESTURE_TAP ? "TAP" : "NONE");

    printf("\n    Long press test...\n");
    ui_gesture_reset(rec);
    ui_gesture_feed_event(rec, &down);
    ui_pointer_event_t long_up = ui_pointer_event_create(POINTER_EVENT_UP, 102, 201, POINTER_TYPE_TOUCH);
    long_up.timestamp_ms = 600;
    g = ui_gesture_feed_event(rec, &long_up);
    printf("    Recognized: %s\n", g == GESTURE_LONG_PRESS ? "LONG_PRESS" : "NONE");

    printf("\n  Click Delay Model:\n");
    ui_click_delay_config_t cfg;
    cfg.mode = CLICK_DELAY_DISABLED;
    ui_click_delay_configure(&cfg);
    printf("    Mode: DISABLED (0ms)\n");
    printf("    Meta: %s\n", ui_click_delay_viewport_meta());
    printf("    CSS:  %s\n", ui_click_delay_touch_action_css());

    printf("\n  Cursor States:\n");
    const char *cursors[] = {"default","pointer","text","move","not-allowed","grab","grabbing"};
    for (int i = 0; i < 7; i++) {
        printf("    %s => cursor: %s;\n", cursors[i], cursors[i]);
    }

    printf("\n  Drag and Drop Model:\n");
    ui_drag_source_t *ds = ui_drag_source_create(DRAG_EFFECT_MOVE, "text/plain", NULL, 0);
    ui_drop_target_t *dt = ui_drop_target_create(DROP_EFFECT_MOVE, "text/plain,text/html");

    ui_drag_source_start(ds, 100, 100, NULL);
    printf("    Drag started at (100,100)\n");
    ui_drag_source_update(ds, 250, 180);
    printf("    Drag moved: offset=(%.0f,%.0f)\n", ds->offset_x, ds->offset_y);

    ui_drop_target_drag_enter(dt, ds);
    printf("    Drop target: accepts? %s\n", ui_drop_target_accepts(dt, ds) ? "Yes" : "No");
    ui_drop_target_drag_over(dt, 250, 180);
    if (ui_drop_target_drop(dt, ds)) printf("    Drop: successful\n");
    ui_drag_source_end(ds);

    ui_drag_source_free(ds);
    ui_drop_target_free(dt);

    printf("\n  Scroll Behavior:\n");
    printf("    scroll-behavior: smooth;\n");
    printf("    overscroll-behavior: contain;\n");

    printf("\n  Touch Action:\n");
    printf("    touch-action: manipulation;\n");
    printf("    touch-action: pan-y pinch-zoom;\n");

    ui_gesture_recognizer_free(rec);
    ui_interaction_shutdown();
}

static void demo_theme_system(void) {
    /* Direct demo without full init to keep it self-contained */
    print_separator("5. THEME SYSTEM (Light/Dark/Custom)");

    printf("  Theme Modes:\n");
    printf("    LIGHT  - Default light color palette\n");
    printf("    DARK   - Inverted dark colors, reduced brightness\n");
    printf("    AUTO   - Follows system @media(prefers-color-scheme)\n");
    printf("    CUSTOM - User-defined theme variables\n");

    printf("\n  Theme Variables (Light mode sample):\n");
    const char *var_pairs_light[][2] = {
        {"--color-bg-primary",   "#ffffff"},
        {"--color-bg-secondary", "#f8fafc"},
        {"--color-text-primary", "#0f172a"},
        {"--color-text-secondary","#64748b"},
        {"--color-border",       "#e2e8f0"},
        {"--color-primary",      "#3b82f6"},
        {"--color-primary-hover","#2563eb"},
        {"--shadow-sm",          "0 1px 2px 0 rgb(0 0 0 / 0.05)"},
        {"--radius-md",          "0.375rem"},
        {"--font-sans",          "system-ui, sans-serif"},
    };
    for (int i = 0; i < 10; i++) {
        printf("    %-26s: %s;\n", var_pairs_light[i][0], var_pairs_light[i][1]);
    }

    printf("\n  Theme Variables (Dark mode overrides):\n");
    const char *var_pairs_dark[][2] = {
        {"--color-bg-primary",   "#0f172a"},
        {"--color-bg-secondary", "#1e293b"},
        {"--color-text-primary", "#f1f5f9"},
        {"--color-text-secondary","#94a3b8"},
        {"--color-border",       "#334155"},
        {"--color-primary",      "#3b82f6"},
        {"--color-primary-hover","#60a5fa"},
        {"--shadow-sm",          "0 1px 2px 0 rgb(0 0 0 / 0.3)"},
        {"--radius-md",          "0.375rem"},
        {"--font-sans",          "system-ui, sans-serif"},
    };
    for (int i = 0; i < 10; i++) {
        printf("    %-26s: %s;\n", var_pairs_dark[i][0], var_pairs_dark[i][1]);
    }

    printf("\n  CSS Generation (Root + Dark Mode):\n");
    printf("  :root {\n");
    printf("    --color-bg-primary: #ffffff;\n");
    printf("    --color-text-primary: #0f172a;\n");
    printf("    /* ... more light variables ... */\n");
    printf("  }\n");
    printf("  @media (prefers-color-scheme: dark) {\n");
    printf("    :root {\n");
    printf("      --color-bg-primary: #0f172a;\n");
    printf("      --color-text-primary: #f1f5f9;\n");
    printf("      /* ... dark overrides ... */\n");
    printf("    }\n");
    printf("  }\n");

    printf("\n  Theme Transition Animation:\n");
    printf("  *, *::before, *::after {\n");
    printf("    transition: background-color 300ms ease-in-out,\n");
    printf("                color 300ms ease-in-out,\n");
    printf("                border-color 300ms ease-in-out;\n");
    printf("  }\n");

    printf("\n  Meta Tags:\n");
    printf("  <meta name=\"color-scheme\" content=\"light dark\">\n");
    printf("  <meta name=\"theme-color\" content=\"#ffffff\">\n");
    printf("  <meta name=\"theme-color\" content=\"#0f172a\"\n");
    printf("        media=\"(prefers-color-scheme: dark)\">\n");

    printf("\n  Theme Provider Strategies:\n");
    printf("    1. CSS Variables: :root { --var: val; }\n");
    printf("    2. Data Attribute: <html data-theme=\"dark\">\n");
    printf("    3. Class Name:     <html class=\"dark\">\n");
    printf("    4. Media Query:    @media (prefers-color-scheme: dark)\n");
}

int main(void) {
    printf("==================================================\n");
    printf("  MINI-UI-UX-HCI  Full Demo Suite\n");
    printf("  C99 UI/UX & HCI Library\n");
    printf("==================================================\n");

    demo_design_tokens();
    demo_component_library();
    demo_accessibility_tree();
    demo_interaction_model();
    demo_theme_system();

    print_separator("ALL DEMOS COMPLETE");
    printf("  Total modules demonstrated: 5\n");
    printf("  - Design Tokens (color/spacing/type/radius/shadow)\n");
    printf("  - Component Library (23 components, variants, states)\n");
    printf("  - Accessibility (ARIA, focus, contrast, WCAG 2.1)\n");
    printf("  - Interaction Model (gestures, drag/drop, pointers)\n");
    printf("  - Theme System (light/dark, CSS vars, transitions)\n");
    printf("\n  Build: make && ./demo\n");
    printf("==================================================\n");
    return 0;
}
