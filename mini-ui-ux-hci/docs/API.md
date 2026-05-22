# mini-ui-ux-hci API Reference

## Architecture

```
src/
├── design_tokens.h     — Design token system (colors, spacing, typography, shadows)
├── component_library.h — Component catalog (23 components with variants & states)
├── accessibility.h     — Accessibility tree, ARIA, focus, contrast, WCAG 2.1
├── interaction_model.h — Touch/pointer events, gestures, drag-and-drop
└── theme_system.h      — Light/dark theming, CSS variable generation
```

## design_tokens.h

### Token Layers

| Layer       | Description | Example |
|-------------|-------------|---------|
| `TOKEN_LAYER_GLOBAL` | Raw primitive values | `--color-blue-500: #3b82f6` |
| `TOKEN_LAYER_ALIAS` | Semantic aliases to globals | `--color-bg-primary: var(--color-white)` |
| `TOKEN_LAYER_COMPONENT` | Component-scoped tokens | `--btn-bg-primary: var(--color-blue-500)` |

### Key Types

```c
typedef enum {
    TOKEN_LAYER_GLOBAL    = 0,
    TOKEN_LAYER_ALIAS     = 1,
    TOKEN_LAYER_COMPONENT = 2,
} ui_token_layer_t;

typedef struct {
    const char *name;           // Token identifier
    ui_token_layer_t layer;     // Token layer
    const char *css_variable;   // e.g. "--color-primary"
    const char *value;          // CSS value string
    const char *inherits_from;  // Parent token name
    const char *dark_value;     // Dark mode override
    bool is_dark_override;
} ui_design_token_t;
```

### Spacing Scale (4px base)

`SPACE_0`(0) `SPACE_1`(4) `SPACE_2`(8) `SPACE_4`(16) `SPACE_6`(24) `SPACE_8`(32) `SPACE_12`(48) `SPACE_16`(64) `SPACE_24`(96) `SPACE_32`(128) `SPACE_48`(192) `SPACE_64`(256)

### Typography Scale

`FONT_SIZE_XS`(12) `FONT_SIZE_SM`(14) `FONT_SIZE_BASE`(16) `FONT_SIZE_LG`(18) `FONT_SIZE_XL`(20) `FONT_SIZE_2XL`(24) `FONT_SIZE_3XL`(30) `FONT_SIZE_4XL`(36) `FONT_SIZE_5XL`(48)

### Key Functions

| Function | Description |
|----------|-------------|
| `ui_tokens_init()` | Initialize global token system |
| `ui_token_create()` | Create a new design token |
| `ui_token_set_dark_override()` | Set dark mode value |
| `ui_token_set_inherits()` | Establish inheritance chain |
| `ui_tokens_to_css()` | Generate CSS custom properties |
| `ui_spacing_to_rem()` | Convert spacing to rem string |
| `ui_typography_to_css()` | Typography token to CSS |
| `ui_shadow_to_css()` | Shadow token to box-shadow CSS |
| `ui_token_collection_create()` | Create token collection |
| `ui_palette_get()` | Get palette color by ID |

## component_library.h

### Component Types

23 components: `BUTTON`, `INPUT`, `CARD`, `MODAL`, `TABLE`, `CHECKBOX`, `RADIO`, `SELECT`, `TEXTAREA`, `TOGGLE`, `TOOLTIP`, `BADGE`, `AVATAR`, `DROPDOWN`, `TABS`, `ACCORDION`, `BREADCRUMB`, `PAGINATION`, `PROGRESS`, `SKELETON`, `TOAST`, `DIALOG`, `DRAWER`

### Variants

`VARIANT_PRIMARY`, `VARIANT_SECONDARY`, `VARIANT_OUTLINE`, `VARIANT_GHOST`, `VARIANT_DESTRUCTIVE`, `VARIANT_LINK`, `VARIANT_SUCCESS`, `VARIANT_WARNING`, `VARIANT_INFO`

### Sizes

`SIZE_XS`, `SIZE_SM`, `SIZE_MD`, `SIZE_LG`, `SIZE_XL`, `SIZE_2XL`, `SIZE_FULL`

### Component States

`STATE_DEFAULT`, `STATE_HOVER`, `STATE_ACTIVE`, `STATE_FOCUS`, `STATE_DISABLED`, `STATE_LOADING`, `STATE_ERROR`, `STATE_SUCCESS`, `STATE_SELECTED`, `STATE_CHECKED`, `STATE_EXPANDED`

### Key Functions

| Function | Description |
|----------|-------------|
| `ui_component_lib_init()` | Initialize component library |
| `ui_component_get()` | Get component by type |
| `ui_component_render_html()` | Render component to HTML |
| `ui_component_render_css()` | Render component CSS |
| `ui_component_class()` | Get CSS class for state |
| `ui_catalog_create()` | Create component catalog |
| `ui_catalog_render_storybook()` | Generate storybook HTML |
| `ui_story_create()` | Create a story entry |
| `ui_props_default()` | Default component props |
| `ui_component_generate_docs()` | Generate markdown docs |

## accessibility.h

### ARIA Roles (76 defined)

Landmarks: `banner`, `main`, `navigation`, `complementary`, `contentinfo`, `form`, `search`, `region`

Widgets: `button`, `checkbox`, `textbox`, `slider`, `switch`, `tab`, `link`, `combobox`

### ARIA Properties (41 defined)

`aria-label`, `aria-expanded`, `aria-selected`, `aria-checked`, `aria-disabled`, `aria-hidden`, `aria-live`, `aria-modal`, `aria-pressed`, `aria-current`, `aria-level`, `aria-valuenow`, `aria-valuemax`, `aria-valuemin`, `aria-haspopup`, `aria-controls`, `aria-describedby`, `aria-labelledby`

### Key Functions

| Function | Description |
|----------|-------------|
| `ui_a11y_node_create()` | Create accessibility tree node |
| `ui_a11y_node_add_child()` | Add child to tree |
| `ui_a11y_tree_to_text()` | Serialize tree as text |
| `ui_a11y_tree_to_html()` | Generate HTML with ARIA attrs |
| `ui_focus_manager_create()` | Create focus manager |
| `ui_focus_set()` | Set focus to node |
| `ui_focus_move()` | Move focus directionally |
| `ui_contrast_check()` | Check contrast ratio |
| `ui_contrast_ratio()` | Calculate WCAG contrast |
| `ui_wcag_report_create()` | Create WCAG audit report |
| `ui_sr_label_create()` | Create screen reader label |
| `ui_keyboard_handle()` | Handle keyboard navigation |

## interaction_model.h

### Pointer Events

`POINTER_EVENT_DOWN`, `POINTER_EVENT_MOVE`, `POINTER_EVENT_UP`, `POINTER_EVENT_CANCEL`, `POINTER_EVENT_OVER`, `POINTER_EVENT_OUT`, `POINTER_EVENT_ENTER`, `POINTER_EVENT_LEAVE`

### Gesture Types

`GESTURE_TAP`, `GESTURE_DOUBLE_TAP`, `GESTURE_LONG_PRESS`, `GESTURE_SWIPE_LEFT/RIGHT/UP/DOWN`, `GESTURE_PINCH_IN/OUT`, `GESTURE_ROTATE`, `GESTURE_PAN`, `GESTURE_FLICK`, `GESTURE_DRAG_START/MOVE/END`

### Key Functions

| Function | Description |
|----------|-------------|
| `ui_gesture_recognizer_create()` | Create gesture recognizer |
| `ui_gesture_feed_event()` | Feed pointer event, returns gesture |
| `ui_gesture_reset()` | Reset recognizer state |
| `ui_click_delay_configure()` | Configure click delay model |
| `ui_interaction_context_create()` | Create interaction context |
| `ui_drag_source_create()` | Create drag source |
| `ui_drop_target_create()` | Create drop target |
| `ui_interaction_render_css()` | Generate hover/focus CSS |

## theme_system.h

### Theme Modes

`THEME_MODE_LIGHT`, `THEME_MODE_DARK`, `THEME_MODE_AUTO`, `THEME_MODE_CUSTOM`

### Provider Strategies

- `THEME_PROVIDER_STRATEGY_CSS_VARS` — CSS custom properties
- `THEME_PROVIDER_STRATEGY_DATA_ATTR` — `data-theme="dark"`
- `THEME_PROVIDER_STRATEGY_CLASS` — `.dark` class
- `THEME_PROVIDER_STRATEGY_MEDIA` — `@media (prefers-color-scheme: dark)`

### Key Functions

| Function | Description |
|----------|-------------|
| `ui_theme_init()` | Initialize theme system |
| `ui_theme_create_default_light()` | Create default light theme |
| `ui_theme_create_default_dark()` | Create default dark theme |
| `ui_theme_register()` | Register a theme |
| `ui_theme_context_create()` | Create theme context |
| `ui_theme_context_toggle()` | Toggle light/dark |
| `ui_theme_to_css()` | Generate CSS from theme |
| `ui_theme_generate_full_css()` | Full root + dark CSS |
| `ui_theme_override_create()` | Component-level override |
| `ui_theme_create_from_template()` | Custom theme from template |
| `ui_theme_transition_create()` | Theme switch animation |
