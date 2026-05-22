# mini-ui-ux-hci Developer Guide

## Overview

`mini-ui-ux-hci` is a C99 library that provides UI/UX design primitives and Human-Computer Interaction patterns. It generates CSS, HTML, and metadata for building accessible, themeable web frontends.

## Project Structure

```
mini-ui-ux-hci/
├── src/                      # Library source
│   ├── design_tokens.h/c     # Design token system
│   ├── component_library.h/c # Component catalog & storybook
│   ├── accessibility.h/c     # ARIA, WCAG, focus management
│   ├── interaction_model.h/c # Gestures, pointer events, drag-drop
│   └── theme_system.h/c      # Light/dark theming, CSS generation
├── examples/                 # Standalone examples
│   ├── 01_design_tokens_demo.c
│   ├── 02_component_form.c
│   └── 03_accessibility_check.c
├── demos/                    # Full demo applications
│   ├── full_ui_kit_demo.c    # All 5 modules demo
│   └── theme_storybook.c     # Theme system + component catalog
├── docs/
│   ├── API.md                # Full API reference
│   └── GUIDE.md              # This guide
├── README.md
└── Makefile
```

## Quick Start

### Build the library

```bash
make
```

### Build and run examples

```bash
make examples
make run-demos
```

### Build individual files

```bash
gcc -std=c99 -Wall -c src/design_tokens.c -o build/design_tokens.o
gcc -std=c99 -Wall -c src/component_library.c -o build/component_library.o
```

## Module 1: Design Tokens

Design tokens are the atomic design decisions: colors, spacing, typography, shadows, animations. They are organized in three layers:

1. **Global** — Raw values (e.g., `#3b82f6`)
2. **Alias** — Semantic references (e.g., `var(--color-blue-500)`)
3. **Component** — Scoped to UI components (e.g., `var(--btn-bg-primary)`)

### Using design tokens

```c
ui_tokens_init();

// Create a token
ui_design_token_t *t = ui_token_create(
    "my-color", TOKEN_LAYER_GLOBAL, "--my-color", "#ff0000");

// Add dark mode override
ui_token_set_dark_override(t, "#cc0000");

// Generate CSS
char css[1024];
ui_tokens_to_css(ui_tokens_global_light(), false, css, sizeof(css));
printf("%s", css);

ui_tokens_shutdown();
```

### Token Inheritance

Tokens can inherit from parent tokens, enabling cascading overrides:

```c
ui_token_create("bg-base",  TOKEN_LAYER_GLOBAL, "--bg-base", "#fff");
ui_token_create("bg-card",  TOKEN_LAYER_ALIAS,  "--bg-card", "var(--bg-base)");
ui_token_create("bg-modal", TOKEN_LAYER_ALIAS,  "--bg-modal","var(--bg-base)");
```

## Module 2: Component Library

The component library provides a catalog of 23 UI components with variants, sizes, and states. It can generate HTML markup, CSS styles, and Storybook-like demo pages.

### Component Props

Every component has these base props:
- **variant**: PRIMARY, SECONDARY, OUTLINE, GHOST, DESTRUCTIVE, LINK
- **size**: XS, SM, MD, LG, XL, 2XL, FULL
- **disabled**: boolean
- **loading**: boolean (shows loading indicator)
- **full_width**: boolean
- **label**: accessible text label

### Creating a button

```c
ui_component_lib_init();

ui_component_t *btn = ui_component_get(COMPONENT_BUTTON);
ui_component_props_t props = ui_props_default(COMPONENT_BUTTON);
props.variant = VARIANT_PRIMARY;
props.size = SIZE_LG;
props.label = "Submit";

char html[256];
ui_render_context_t ctx = {RENDER_TARGET_HTML, false, false, false, 0};
ui_component_render_html(btn, &props, &ctx, html, sizeof(html));

ui_component_lib_shutdown();
```

### Building a Storybook

```c
ui_component_catalog_t *cat = ui_catalog_create("My Storybook", "1.0.0");
ui_catalog_add_component(cat, ui_component_get(COMPONENT_BUTTON));
ui_catalog_add_component(cat, ui_component_get(COMPONENT_INPUT));
ui_catalog_add_component(cat, ui_component_get(COMPONENT_CARD));

char storybook[65536];
ui_catalog_render_storybook(cat, &ctx, storybook, sizeof(storybook));
// Write storybook to .html file
```

## Module 3: Accessibility

The accessibility system manages ARIA roles, keyboard navigation, focus management, color contrast checking, and WCAG 2.1 compliance.

### Building an accessibility tree

```c
ui_a11y_node_t *page = ui_a11y_node_create("Page", ARIA_ROLE_MAIN);
ui_a11y_node_t *nav  = ui_a11y_node_create("Navigation", ARIA_ROLE_NAVIGATION);
ui_a11y_node_t *btn  = ui_a11y_node_create("Search", ARIA_ROLE_BUTTON);

ui_a11y_node_add_child(page, nav);
ui_a11y_node_add_child(nav, btn);

// Assign tab order
int tab = 1;
ui_a11y_assign_tab_order(page, &tab);

// Focus management
ui_focus_manager_t *fm = ui_focus_manager_create();
ui_focus_set(fm, btn);
ui_focus_move(fm, FOCUS_DIRECTION_NEXT);
```

### Checking contrast

```c
ui_contrast_result_t res = ui_contrast_check_hex("#0f172a", "#ffffff");
printf("Ratio: %.2f:1 | AA: %s\n", res.ratio,
       res.passes_aa_normal ? "PASS" : "FAIL");
```

Contrast thresholds:
- **AA Large** (18px+ bold or 24px+): 3.0:1
- **AA Normal** (< 18px): 4.5:1
- **AAA Large**: 4.5:1
- **AAA Normal**: 7.0:1

### WCAG 2.1 Report

```c
ui_wcag_report_t *report = ui_wcag_report_create(WCAG_LEVEL_AA, "https://mysite.com");
ui_wcag_check_all(report, root_node);
char summary[1024];
ui_wcag_report_summary(report, summary, sizeof(summary));
```

## Module 4: Interaction Model

Handles pointer events, gesture recognition, cursor management, and drag-and-drop.

### Gesture recognition

```c
ui_gesture_recognizer_t *rec = ui_gesture_recognizer_create();

ui_pointer_event_t down = ui_pointer_event_create(POINTER_EVENT_DOWN, 100, 200, POINTER_TYPE_TOUCH);
ui_pointer_event_t move = ui_pointer_event_create(POINTER_EVENT_MOVE, 250, 200, POINTER_TYPE_TOUCH);
ui_pointer_event_t up   = ui_pointer_event_create(POINTER_EVENT_UP, 250, 200, POINTER_TYPE_TOUCH);

ui_gesture_feed_event(rec, &down);
ui_gesture_type_t g = ui_gesture_feed_event(rec, &move);
// g == GESTURE_SWIPE_RIGHT if velocity exceeds threshold
ui_gesture_feed_event(rec, &up);
```

### Click delay elimination

```c
ui_click_delay_config_t cfg = {CLICK_DELAY_DISABLED, 0, true, true, 0.5f};
ui_click_delay_configure(&cfg);
printf("%s", ui_click_delay_viewport_meta());
```

### Drag and drop

```c
ui_drag_source_t *ds = ui_drag_source_create(DRAG_EFFECT_MOVE, "text/plain", data, len);
ui_drop_target_t *dt = ui_drop_target_create(DROP_EFFECT_MOVE, "text/plain");

ui_drag_source_start(ds, 100, 100, element);
ui_drop_target_drag_enter(dt, ds);
if (ui_drop_target_drop(dt, ds)) { /* handle drop */ }
ui_drag_source_end(ds);
```

## Module 5: Theme System

The theme system manages light/dark mode switching, CSS variable generation, theme inheritance, and component-level overrides.

### Basic light/dark switching

```c
ui_theme_init();

ui_theme_context_t *ctx = ui_theme_context_root();
ui_theme_context_light(ctx);   // Switch to light
ui_theme_context_dark(ctx);    // Switch to dark
ui_theme_context_toggle(ctx);  // Toggle

ui_theme_shutdown();
```

### Generating theme CSS

```c
ui_theme_css_options_t opts = {
    .include_root_selector = true,
    .include_dark_mode = true,
    .include_color_scheme = true,
    .include_transition = true,
    .nest_dark_media = true,
};

char css[16384];
ui_theme_generate_full_css(ctx, &opts, css, sizeof(css));
```

### Component-level theme override

```c
ui_theme_override_t *ov = ui_theme_override_create(".my-card", OVERRIDE_SCOPE_CHILDREN);
ui_theme_override_add_var(ov, "--card-bg", "#fef3c7", "#78350f");
ui_theme_override_add_var(ov, "--card-border", "#f59e0b", "#d97706");
ui_theme_override_apply(ov);
```

### Custom theme from template

```c
ui_custom_theme_template_t tpl = {
    .name = "my-theme",
    .base_theme_name = "default-light",
    .font_family = "Inter, sans-serif",
    .border_radius_scale = "rounded",
};

ui_theme_t *custom = ui_theme_create_from_template(&tpl);
ui_theme_register(custom);
```

### Theme transition animation

```c
ui_theme_transition_t tr = ui_theme_transition_create(THEME_TRANSITION_FADE, 300);
tr.easing = "ease-in-out";

char css[512];
ui_theme_transition_to_css(&tr, css, sizeof(css));
// Wraps all elements with smooth color transitions
```

## Memory Management

All `_create()` functions return heap-allocated structures. Corresponding `_free()` functions must be called to release memory:

```c
ui_tokens_shutdown();           // Frees all tokens
ui_component_lib_shutdown();    // Frees all components
ui_a11y_shutdown();             // Frees a11y state
ui_interaction_shutdown();      // Frees interaction state
ui_theme_shutdown();            // Frees all themes
```

Or free individually:
```c
ui_design_token_t *t = ui_token_create(...);
// ... use t ...
// Tokens are owned by the global registry; freed via ui_tokens_shutdown()

ui_a11y_node_t *node = ui_a11y_node_create(...);
ui_a11y_node_free(node);

ui_theme_t *theme = ui_theme_create(...);
ui_theme_free(theme);
```

## Thread Safety

The library is **not thread-safe**. All operations should be performed from a single thread, typically the main UI thread. Initialize before use, render from the same thread, and shut down after.

## Integration Pattern

A typical application lifecycle:

```c
int main(void) {
    // 1. Initialize all subsystems
    ui_tokens_init();
    ui_component_lib_init();
    ui_theme_init();
    ui_a11y_init();
    ui_interaction_init();

    // 2. Build UI
    // Create accessibility tree
    // Register components
    // Set up gesture recognizers

    // 3. Generate output
    char css[65536], html[65536];
    ui_theme_generate_full_css(ctx, &opts, css, sizeof(css));
    ui_catalog_render_storybook(catalog, &ctx, html, sizeof(html));
    // Write to files, serve over HTTP, etc.

    // 4. Shutdown
    ui_interaction_shutdown();
    ui_a11y_shutdown();
    ui_theme_shutdown();
    ui_component_lib_shutdown();
    ui_tokens_shutdown();
    return 0;
}
```
