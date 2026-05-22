#ifndef MINI_UI_COMPONENT_LIBRARY_H
#define MINI_UI_COMPONENT_LIBRARY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ===== Forward Declarations ===== */
typedef struct ui_component_s      ui_component_t;
typedef struct ui_component_catalog_s ui_component_catalog_t;
typedef struct ui_component_props_s ui_component_props_t;
typedef struct ui_component_states_s ui_component_states_t;
typedef struct ui_component_story_s ui_component_story_t;

/* ===== Component Types ===== */
typedef enum {
    COMPONENT_BUTTON       = 0,
    COMPONENT_INPUT        = 1,
    COMPONENT_CARD         = 2,
    COMPONENT_MODAL        = 3,
    COMPONENT_TABLE        = 4,
    COMPONENT_CHECKBOX     = 5,
    COMPONENT_RADIO        = 6,
    COMPONENT_SELECT       = 7,
    COMPONENT_TEXTAREA     = 8,
    COMPONENT_TOGGLE       = 9,
    COMPONENT_TOOLTIP      = 10,
    COMPONENT_BADGE        = 11,
    COMPONENT_AVATAR       = 12,
    COMPONENT_DROPDOWN     = 13,
    COMPONENT_TABS         = 14,
    COMPONENT_ACCORDION    = 15,
    COMPONENT_BREADCRUMB   = 16,
    COMPONENT_PAGINATION   = 17,
    COMPONENT_PROGRESS     = 18,
    COMPONENT_SKELETON     = 19,
    COMPONENT_TOAST        = 20,
    COMPONENT_DIALOG       = 21,
    COMPONENT_DRAWER       = 22,
    COMPONENT_COUNT        = 23,
} ui_component_type_t;

/* ===== Variant Enums ===== */
typedef enum {
    VARIANT_PRIMARY    = 0,
    VARIANT_SECONDARY  = 1,
    VARIANT_OUTLINE    = 2,
    VARIANT_GHOST      = 3,
    VARIANT_DESTRUCTIVE = 4,
    VARIANT_LINK        = 5,
    VARIANT_SUCCESS     = 6,
    VARIANT_WARNING     = 7,
    VARIANT_INFO        = 8,
} ui_variant_t;

typedef enum {
    SIZE_XS  = 0,
    SIZE_SM  = 1,
    SIZE_MD  = 2,
    SIZE_LG  = 3,
    SIZE_XL  = 4,
    SIZE_2XL = 5,
    SIZE_FULL = 6,
} ui_size_t;

/* ===== Component State ===== */
typedef enum {
    STATE_DEFAULT  = 0,
    STATE_HOVER    = 1,
    STATE_ACTIVE   = 2,
    STATE_FOCUS    = 3,
    STATE_DISABLED = 4,
    STATE_LOADING  = 5,
    STATE_ERROR    = 6,
    STATE_SUCCESS  = 7,
    STATE_SELECTED = 8,
    STATE_CHECKED  = 9,
    STATE_EXPANDED = 10,
} ui_component_state_t;

/* ===== Component Props ===== */
struct ui_component_props_s {
    ui_variant_t  variant;
    ui_size_t     size;
    bool          disabled;
    bool          loading;
    bool          full_width;
    const char   *label;
    const char   *placeholder;
    const char   *icon_left;
    const char   *icon_right;
    const char   *tooltip;
    const char   *id;
    const char   *class_name;
    int           tab_index;
    bool          auto_focus;
    void         *user_data;
};

/* ===== Component States (CSS classes mapping) ===== */
struct ui_component_states_s {
    const char *default_class;
    const char *hover_class;
    const char *active_class;
    const char *focus_class;
    const char *disabled_class;
    const char *loading_class;
    ui_component_state_t current;
};

/* ===== Component Definition ===== */
struct ui_component_s {
    ui_component_type_t    type;
    const char            *name;
    const char            *tag;           /* HTML tag e.g. "button", "div" */
    const char            *category;      /* "form", "layout", "feedback" */
    ui_component_props_t   props;
    ui_component_states_t  states;
    const char            *description;
    const char            *css_base;      /* base CSS rules */
    const char            *css_variants;  /* variant-specific CSS */
    const char            *css_sizes;     /* size-specific CSS */
    const char           **aria_roles;
    int                    aria_role_count;
    bool                   is_interactive;
    bool                   is_container;
};

/* ===== Story (Storybook-like demo entry) ===== */
struct ui_component_story_s {
    const char          *name;
    const char          *description;
    ui_component_type_t  component_type;
    ui_component_props_t props;
    const char          *html_output;
    const char          *css_output;
    const char          *notes;          /* usage notes / accessibility notes */
    struct ui_component_story_s **children;
    int                  child_count;
    int                  child_capacity;
};

/* ===== Component Catalog ===== */
struct ui_component_catalog_s {
    ui_component_t    **components;
    int                 component_count;
    int                 component_capacity;
    ui_component_story_t **stories;
    int                 story_count;
    int                 story_capacity;
    const char         *name;
    const char         *version;
};

/* ===== Button Specific ===== */
typedef enum {
    BUTTON_TYPE_BUTTON  = 0,
    BUTTON_TYPE_SUBMIT  = 1,
    BUTTON_TYPE_RESET   = 2,
    BUTTON_TYPE_MENU    = 3,
} ui_button_type_t;

/* ===== Input Specific ===== */
typedef enum {
    INPUT_TYPE_TEXT     = 0,
    INPUT_TYPE_PASSWORD = 1,
    INPUT_TYPE_EMAIL    = 2,
    INPUT_TYPE_NUMBER   = 3,
    INPUT_TYPE_SEARCH   = 4,
    INPUT_TYPE_TEL      = 5,
    INPUT_TYPE_URL      = 6,
    INPUT_TYPE_DATE     = 7,
    INPUT_TYPE_COLOR    = 8,
    INPUT_TYPE_FILE     = 9,
} ui_input_type_t;

/* ===== Table Specific ===== */
typedef enum {
    TABLE_DENSITY_COMPACT   = 0,
    TABLE_DENSITY_DEFAULT   = 1,
    TABLE_DENSITY_COMFORTABLE = 2,
} ui_table_density_t;

typedef struct ui_table_column_s {
    const char *header;
    const char *accessor;       /* data key */
    const char *align;          /* "left", "center", "right" */
    int         width_pct;      /* percentage 0-100, 0 = auto */
    bool        sortable;
    bool        filterable;
    bool        resizable;
} ui_table_column_t;

/* ===== Modal/Dialog Specific ===== */
typedef enum {
    MODAL_SIZE_SM  = 0,
    MODAL_SIZE_MD  = 1,
    MODAL_SIZE_LG  = 2,
    MODAL_SIZE_XL  = 3,
    MODAL_SIZE_FULL = 4,
} ui_modal_size_t;

/* ===== Render Context ===== */
typedef enum {
    RENDER_TARGET_HTML   = 0,
    RENDER_TARGET_CSS    = 1,
    RENDER_TARGET_REACT  = 2,
    RENDER_TARGET_VUE    = 3,
    RENDER_TARGET_SVELTE = 4,
} ui_render_target_t;

typedef struct {
    ui_render_target_t target;
    bool               minify;
    bool               include_comments;
    bool               dark_mode;
    int                indent_level;
} ui_render_context_t;

/* ===== API ===== */

/* Initialize component library */
void ui_component_lib_init(void);
void ui_component_lib_shutdown(void);

/* Create a new component definition */
ui_component_t* ui_component_create(
    ui_component_type_t type,
    const char         *name,
    const char         *category);

/* Get a component by type */
ui_component_t* ui_component_get(ui_component_type_t type);

/* Get a component by name */
ui_component_t* ui_component_get_by_name(const char *name);

/* Render a component to HTML string */
int ui_component_render_html(
    const ui_component_t      *comp,
    const ui_component_props_t *props,
    const ui_render_context_t  *ctx,
    char                       *buffer,
    int                         buffer_size);

/* Render a component's CSS */
int ui_component_render_css(
    const ui_component_t      *comp,
    const ui_component_props_t *props,
    const ui_render_context_t  *ctx,
    char                       *buffer,
    int                         buffer_size);

/* Get CSS class name for a component+state combination */
const char* ui_component_class(
    const ui_component_t *comp,
    ui_component_state_t  state);

/* Create a catalog */
ui_component_catalog_t* ui_catalog_create(const char *name, const char *version);

/* Add component to catalog */
void ui_catalog_add_component(
    ui_component_catalog_t *catalog,
    ui_component_t         *component);

/* Add story to catalog */
void ui_catalog_add_story(
    ui_component_catalog_t *catalog,
    ui_component_story_t   *story);

/* Create a story */
ui_component_story_t* ui_story_create(
    const char          *name,
    ui_component_type_t  comp_type,
    ui_component_props_t props);

/* Add child story (composition example) */
void ui_story_add_child(
    ui_component_story_t *parent,
    ui_component_story_t *child);

/* Render full storybook HTML */
int ui_catalog_render_storybook(
    const ui_component_catalog_t *catalog,
    const ui_render_context_t    *ctx,
    char                         *buffer,
    int                           buffer_size);

/* Render a component state matrix (all variant x size x state combos) */
int ui_component_render_state_matrix(
    const ui_component_t     *comp,
    const ui_render_context_t *ctx,
    char                      *buffer,
    int                        buffer_size);

/* Get list of all component names */
int ui_component_list_names(const char ***names);

/* Get list of all component categories */
int ui_catalog_list_categories(const char ***categories);

/* Check if a component type is a container */
bool ui_component_is_container(ui_component_type_t type);

/* Get recommended ARIA roles for component */
const char** ui_component_aria_roles(
    ui_component_type_t type, int *count);

/* Generate component documentation (markdown) */
int ui_component_generate_docs(
    const ui_component_t *comp,
    char *buffer, int buf_size);

/* Create default props for a component type */
ui_component_props_t ui_props_default(ui_component_type_t type);

/* Create props with specified variant */
ui_component_props_t ui_props_with_variant(
    ui_component_type_t type, ui_variant_t variant);

/* Create props with specified size */
ui_component_props_t ui_props_with_size(
    ui_component_type_t type, ui_size_t size);

/* Free a component */
void ui_component_free(ui_component_t *comp);

/* Free a catalog */
void ui_catalog_free(ui_component_catalog_t *catalog);

/* Free a story */
void ui_story_free(ui_component_story_t *story);

#endif /* MINI_UI_COMPONENT_LIBRARY_H */
