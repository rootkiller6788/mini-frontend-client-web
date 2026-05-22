#ifndef MINI_UI_THEME_SYSTEM_H
#define MINI_UI_THEME_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ===== Theme Mode ===== */
typedef enum {
    THEME_MODE_LIGHT = 0,
    THEME_MODE_DARK  = 1,
    THEME_MODE_AUTO  = 2,   /* follow system preference */
    THEME_MODE_CUSTOM = 3,  /* user-defined custom theme */
} ui_theme_mode_t;

/* ===== Color Scheme ===== */
typedef enum {
    COLOR_SCHEME_NORMAL     = 0,
    COLOR_SCHEME_LIGHT      = 1,
    COLOR_SCHEME_DARK       = 2,
    COLOR_SCHEME_LIGHT_DARK = 3,  /* supports both */
    COLOR_SCHEME_ONLY_LIGHT = 4,
    COLOR_SCHEME_ONLY_DARK  = 5,
} ui_color_scheme_t;

/* ===== Theme Variable (CSS custom property) ===== */
typedef struct {
    const char *name;          /* e.g. "--color-bg-primary" */
    const char *light_value;   /* value in light mode */
    const char *dark_value;    /* value in dark mode, NULL = same as light */
    const char *description;   /* human-readable description */
} ui_theme_variable_t;

/* ===== Theme Definition ===== */
#define UI_THEME_MAX_VARS 512
#define UI_THEME_NAME_MAX 64

typedef struct {
    char                name[UI_THEME_NAME_MAX];
    char                description[256];
    ui_theme_mode_t     mode;
    bool                is_default;
    ui_theme_variable_t *variables;
    int                 var_count;
    int                 var_capacity;
    ui_color_scheme_t   color_scheme;
    bool                is_readonly;   /* built-in themes */
    void               *user_data;
} ui_theme_t;

/* ===== Theme Provider ===== */
typedef enum {
    THEME_PROVIDER_STRATEGY_CSS_VARS  = 0,  /* CSS custom properties */
    THEME_PROVIDER_STRATEGY_DATA_ATTR = 1,  /* data-theme="dark" */
    THEME_PROVIDER_STRATEGY_CLASS     = 2,  /* .dark class on <html> */
    THEME_PROVIDER_STRATEGY_MEDIA     = 3,  /* @media (prefers-color-scheme) */
} ui_theme_provider_strategy_t;

typedef struct {
    ui_theme_provider_strategy_t strategy;
    const char                   *attribute_name;  /* e.g. "data-theme" */
    const char                   *class_name;      /* e.g. "dark" */
    const char                   *css_selector;    /* e.g. "[data-theme='dark']" */
    bool                          use_local_storage;
    const char                   *storage_key;
} ui_theme_provider_config_t;

/* ===== Theme Context ===== */
typedef struct ui_theme_context_s {
    ui_theme_t        *active_theme;
    ui_theme_t        *fallback_theme;    /* if active fails */
    ui_theme_mode_t    mode;
    bool               is_dark;
    bool               use_system_preference;
    ui_theme_provider_config_t provider_config;
    int                transition_duration_ms;
    const char        *transition_easing;  /* CSS easing function */
    struct ui_theme_context_s *parent;     /* nested theme context */
    int                ref_count;
    void              *user_data;
} ui_theme_context_t;

/* ===== Theme Override (per component) ===== */
typedef enum {
    OVERRIDE_SCOPE_COMPONENT = 0, /* only this component */
    OVERRIDE_SCOPE_CHILDREN  = 1, /* this + all descendants */
    OVERRIDE_SCOPE_GLOBAL    = 2, /* entire document */
} ui_theme_override_scope_t;

typedef struct {
    const char                *selector;     /* CSS selector to target */
    ui_theme_override_scope_t  scope;
    ui_theme_variable_t       *overrides;
    int                        override_count;
    int                        override_capacity;
    bool                       active;
} ui_theme_override_t;

/* ===== System Preference Detection ===== */
typedef struct {
    bool     supports_prefers_color_scheme;
    bool     prefers_dark;
    bool     prefers_light;
    bool     prefers_contrast_more;
    bool     prefers_contrast_less;
    bool     prefers_reduced_motion;
    bool     prefers_reduced_transparency;
    bool     prefers_reduced_data;
    bool     forced_colors_active;
    bool     supports_color_scheme_meta;
} ui_system_preferences_t;

/* ===== Custom Theme Template ===== */
typedef struct {
    const char *name;
    const char *description;
    const char *base_theme_name;   /* inherit from */
    struct {
        const char *key;    /* e.g. "color.bg.primary" */
        const char *light;  /* CSS value for light */
        const char *dark;   /* CSS value for dark */
    } *color_overrides;
    int  override_count;
    const char *font_family;
    const char *border_radius_scale;  /* "sharp", "rounded", "pill" */
    const char *spacing_scale;       /* "compact", "comfortable", "spacious" */
} ui_custom_theme_template_t;

/* ===== Theme Transition Animation ===== */
typedef enum {
    THEME_TRANSITION_NONE     = 0,
    THEME_TRANSITION_FADE     = 1,
    THEME_TRANSITION_SLIDE    = 2,
    THEME_TRANSITION_DISSOLVE = 3,
    THEME_TRANSITION_CIRCLE   = 4,  /* circular reveal from cursor */
} ui_theme_transition_type_t;

typedef struct {
    ui_theme_transition_type_t type;
    int                        duration_ms;
    const char                *easing;
    float                      origin_x;   /* for circular reveal (0-1) */
    float                      origin_y;
} ui_theme_transition_t;

/* ===== Theme Schedule (time-based switching) ===== */
typedef struct {
    bool     enabled;
    int      light_hour;   /* 0-23, when to switch to light */
    int      dark_hour;    /* 0-23, when to switch to dark */
    bool     follow_sunset; /* use geolocation sunset data */
    double   latitude;
    double   longitude;
} ui_theme_schedule_t;

/* ===== Theme Storage ===== */
typedef enum {
    THEME_STORAGE_NONE       = 0,
    THEME_STORAGE_LOCAL      = 1,
    THEME_STORAGE_SESSION    = 2,
    THEME_STORAGE_COOKIE     = 3,
} ui_theme_storage_backend_t;

typedef struct {
    ui_theme_storage_backend_t backend;
    const char                 *key;        /* storage key */
    int                         max_age_days; /* for cookies */
} ui_theme_storage_t;

/* ===== Theme Manager (global singleton) ===== */
typedef struct {
    ui_theme_t          **themes;
    int                   theme_count;
    int                   theme_capacity;
    ui_theme_context_t   *root_context;
    ui_theme_override_t  *active_overrides;
    int                   override_count;
    int                   override_capacity;
    ui_system_preferences_t system_prefs;
    ui_theme_schedule_t   schedule;
    ui_theme_storage_t    storage;
    bool                  initialized;
} ui_theme_manager_t;

/* ===== CSS Generation Options ===== */
typedef struct {
    bool  minify;
    bool  include_root_selector;  /* :root { } wrapper */
    bool  include_dark_mode;      /* include dark mode variables */
    bool  include_transition;     /* include transition CSS */
    bool  include_color_scheme;   /* include color-scheme property */
    bool  nest_dark_media;        /* @media (prefers-color-scheme: dark) */
    bool  use_layers;             /* @layer theme { } */
    int   indent_spaces;
} ui_theme_css_options_t;

/* ===== API ===== */

/* --- Initialization --- */

/* Initialize theme system */
void ui_theme_init(void);

/* Initialize with custom defaults */
void ui_theme_init_with_config(const ui_theme_provider_config_t *config);

/* Shutdown theme system */
void ui_theme_shutdown(void);

/* --- System Preferences --- */

/* Detect system color scheme preferences */
ui_system_preferences_t ui_theme_detect_system_prefs(void);

/* Get current effective color scheme */
ui_color_scheme_t ui_theme_get_color_scheme(void);

/* Check if dark mode is preferred by system */
bool ui_theme_system_prefers_dark(void);

/* --- Theme Provider --- */

/* Get the global theme manager */
ui_theme_manager_t* ui_theme_manager_get(void);

/* Register a theme with the manager */
void ui_theme_register(ui_theme_t *theme);

/* Unregister a theme */
void ui_theme_unregister(const char *theme_name);

/* Get a theme by name */
ui_theme_t* ui_theme_get(const char *name);

/* List all registered theme names */
int ui_theme_list_names(const char ***names);

/* --- Theme Context --- */

/* Create a theme context */
ui_theme_context_t* ui_theme_context_create(ui_theme_mode_t mode);

/* Set the active theme on a context */
void ui_theme_context_set_theme(
    ui_theme_context_t *ctx,
    ui_theme_t         *theme);

/* Switch to light mode */
void ui_theme_context_light(ui_theme_context_t *ctx);

/* Switch to dark mode */
void ui_theme_context_dark(ui_theme_context_t *ctx);

/* Toggle between light and dark */
void ui_theme_context_toggle(ui_theme_context_t *ctx);

/* Create child context (for component-level theming) */
ui_theme_context_t* ui_theme_context_create_child(
    ui_theme_context_t *parent);

/* Get root context */
ui_theme_context_t* ui_theme_context_root(void);

/* Get effective mode (resolves AUTO to light/dark) */
ui_theme_mode_t ui_theme_context_effective_mode(
    const ui_theme_context_t *ctx);

/* Check if currently dark */
bool ui_theme_context_is_dark(const ui_theme_context_t *ctx);

/* Free theme context (ref-counted) */
void ui_theme_context_free(ui_theme_context_t *ctx);

/* --- Theme Variables --- */

/* Create a theme variable */
ui_theme_variable_t* ui_theme_var_create(
    const char *name,
    const char *light_value,
    const char *dark_value);

/* Add variable to a theme */
void ui_theme_add_var(ui_theme_t *theme, ui_theme_variable_t *var);

/* Remove variable from theme */
void ui_theme_remove_var(ui_theme_t *theme, const char *name);

/* Get variable value for the current mode */
const char* ui_theme_var_get(
    const ui_theme_t *theme,
    const char       *var_name,
    bool              is_dark);

/* --- Theme Creation --- */

/* Create a new theme */
ui_theme_t* ui_theme_create(
    const char     *name,
    ui_theme_mode_t mode);

/* Create a default light theme with all standard variables */
ui_theme_t* ui_theme_create_default_light(void);

/* Create a default dark theme with all standard variables */
ui_theme_t* ui_theme_create_default_dark(void);

/* Create a theme from a template */
ui_theme_t* ui_theme_create_from_template(
    const ui_custom_theme_template_t *template_def);

/* Clone an existing theme */
ui_theme_t* ui_theme_clone(const ui_theme_t *source, const char *new_name);

/* Free a theme */
void ui_theme_free(ui_theme_t *theme);

/* --- Theme Override --- */

/* Create a component-level theme override */
ui_theme_override_t* ui_theme_override_create(
    const char                *selector,
    ui_theme_override_scope_t  scope);

/* Add an override variable */
void ui_theme_override_add_var(
    ui_theme_override_t *override,
    const char          *name,
    const char          *light_value,
    const char          *dark_value);

/* Apply an override (makes it active) */
void ui_theme_override_apply(ui_theme_override_t *override);

/* Remove an override */
void ui_theme_override_remove(ui_theme_override_t *override);

/* Remove all overrides */
void ui_theme_override_clear_all(void);

/* Free an override */
void ui_theme_override_free(ui_theme_override_t *override);

/* --- CSS Generation --- */

/* Generate CSS custom properties from a theme */
int ui_theme_to_css(
    const ui_theme_t            *theme,
    const ui_theme_css_options_t *options,
    char *buffer, int buf_size);

/* Generate CSS for theme transition animation */
int ui_theme_transition_to_css(
    const ui_theme_transition_t *transition,
    char *buffer, int buf_size);

/* Generate the full theme CSS (root + dark + transitions) */
int ui_theme_generate_full_css(
    const ui_theme_context_t    *ctx,
    const ui_theme_css_options_t *options,
    char *buffer, int buf_size);

/* Generate only the dark mode CSS */
int ui_theme_generate_dark_css(
    const ui_theme_t            *theme,
    const ui_theme_css_options_t *options,
    char *buffer, int buf_size);

/* --- Theme Storage --- */

/* Save current theme mode to storage */
bool ui_theme_save_mode(ui_theme_mode_t mode);

/* Load saved theme mode from storage */
ui_theme_mode_t ui_theme_load_mode(void);

/* Save custom theme to storage */
bool ui_theme_save_custom(const ui_theme_t *theme);

/* Load custom theme from storage */
ui_theme_t* ui_theme_load_custom(const char *name);

/* Clear stored theme data */
void ui_theme_storage_clear(void);

/* --- Theme Schedule --- */

/* Configure time-based theme switching */
void ui_theme_schedule_configure(const ui_theme_schedule_t *schedule);

/* Check if theme should switch based on schedule */
bool ui_theme_schedule_should_switch(const ui_theme_schedule_t *schedule);

/* --- Meta Tags --- */

/* Generate <meta name="color-scheme"> tag */
const char* ui_theme_meta_color_scheme(ui_color_scheme_t scheme);

/* Generate <meta name="theme-color"> tag */
const char* ui_theme_meta_theme_color(const char *hex_color);

/* Generate <meta name="theme-color" media="(prefers-color-scheme: dark)"> */
const char* ui_theme_meta_theme_color_dark(const char *hex_color);

/* --- Transition Animations --- */

/* Create a theme transition */
ui_theme_transition_t ui_theme_transition_create(
    ui_theme_transition_type_t type,
    int duration_ms);

/* Start theme transition (fires CSS animation) */
void ui_theme_transition_start(const ui_theme_transition_t *transition);

/* --- Utility --- */

/* Get CSS class name for a theme mode */
const char* ui_theme_mode_class(ui_theme_mode_t mode);

/* Get data attribute value for a theme mode */
const char* ui_theme_mode_data_attr(ui_theme_mode_t mode);

/* Validate theme variable name (must start with --) */
bool ui_theme_var_name_valid(const char *name);

/* Merge two themes (base + overrides = result) */
ui_theme_t* ui_theme_merge(
    const ui_theme_t *base,
    const ui_theme_t *overrides,
    const char       *result_name);

#endif /* MINI_UI_THEME_SYSTEM_H */
