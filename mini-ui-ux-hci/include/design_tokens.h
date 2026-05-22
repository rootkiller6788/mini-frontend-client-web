#ifndef MINI_UI_DESIGN_TOKENS_H
#define MINI_UI_DESIGN_TOKENS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ===== Token Layer Enums ===== */
typedef enum {
    TOKEN_LAYER_GLOBAL     = 0,
    TOKEN_LAYER_ALIAS      = 1,
    TOKEN_LAYER_COMPONENT  = 2
} ui_token_layer_t;

/* ===== Color ===== */
typedef struct {
    const char *name;
    uint32_t hex;       /* 0xAABBGGRR or 0xRRGGBBAA depending on endian */
    uint8_t r, g, b, a;
} ui_color_t;

typedef enum {
    COLOR_SLATE_50   = 0,  COLOR_SLATE_100,  COLOR_SLATE_200,
    COLOR_SLATE_300,  COLOR_SLATE_400,  COLOR_SLATE_500,
    COLOR_SLATE_600,  COLOR_SLATE_700,  COLOR_SLATE_800,
    COLOR_SLATE_900,  COLOR_SLATE_950,
    COLOR_GRAY_50,    COLOR_GRAY_100,   COLOR_GRAY_200,
    COLOR_GRAY_300,   COLOR_GRAY_400,   COLOR_GRAY_500,
    COLOR_GRAY_600,   COLOR_GRAY_700,   COLOR_GRAY_800,
    COLOR_GRAY_900,   COLOR_GRAY_950,
    COLOR_RED_50,     COLOR_RED_100,    COLOR_RED_200,
    COLOR_RED_300,    COLOR_RED_400,    COLOR_RED_500,
    COLOR_RED_600,    COLOR_RED_700,    COLOR_RED_800,
    COLOR_RED_900,    COLOR_RED_950,
    COLOR_BLUE_50,    COLOR_BLUE_100,   COLOR_BLUE_200,
    COLOR_BLUE_300,   COLOR_BLUE_400,   COLOR_BLUE_500,
    COLOR_BLUE_600,   COLOR_BLUE_700,   COLOR_BLUE_800,
    COLOR_BLUE_900,   COLOR_BLUE_950,
    COLOR_GREEN_50,   COLOR_GREEN_100,  COLOR_GREEN_200,
    COLOR_GREEN_300,  COLOR_GREEN_400,  COLOR_GREEN_500,
    COLOR_GREEN_600,  COLOR_GREEN_700,  COLOR_GREEN_800,
    COLOR_GREEN_900,  COLOR_GREEN_950,
    COLOR_YELLOW_50,  COLOR_YELLOW_100, COLOR_YELLOW_200,
    COLOR_YELLOW_300, COLOR_YELLOW_400, COLOR_YELLOW_500,
    COLOR_YELLOW_600, COLOR_YELLOW_700, COLOR_YELLOW_800,
    COLOR_YELLOW_900, COLOR_YELLOW_950,
    COLOR_PURPLE_50,  COLOR_PURPLE_100, COLOR_PURPLE_200,
    COLOR_PURPLE_300, COLOR_PURPLE_400, COLOR_PURPLE_500,
    COLOR_PURPLE_600, COLOR_PURPLE_700, COLOR_PURPLE_800,
    COLOR_PURPLE_900, COLOR_PURPLE_950,
    COLOR_WHITE,      COLOR_BLACK,      COLOR_TRANSPARENT,
    COLOR_COUNT
} ui_color_id_t;

/* ===== Spacing Scale (4px base) ===== */
typedef enum {
    SPACE_0    = 0,
    SPACE_PX   = 1,
    SPACE_0_5  = 2,   /* 2px */
    SPACE_1    = 4,
    SPACE_1_5  = 6,
    SPACE_2    = 8,
    SPACE_2_5  = 10,
    SPACE_3    = 12,
    SPACE_3_5  = 14,
    SPACE_4    = 16,
    SPACE_5    = 20,
    SPACE_6    = 24,
    SPACE_7    = 28,
    SPACE_8    = 32,
    SPACE_9    = 36,
    SPACE_10   = 40,
    SPACE_11   = 44,
    SPACE_12   = 48,
    SPACE_14   = 56,
    SPACE_16   = 64,
    SPACE_20   = 80,
    SPACE_24   = 96,
    SPACE_28   = 112,
    SPACE_32   = 128,
    SPACE_36   = 144,
    SPACE_40   = 160,
    SPACE_44   = 176,
    SPACE_48   = 192,
    SPACE_52   = 208,
    SPACE_56   = 224,
    SPACE_60   = 240,
    SPACE_64   = 256,
    SPACE_72   = 288,
    SPACE_80   = 320,
    SPACE_96   = 384,
} ui_spacing_t;

/* ===== Typography Scale ===== */
typedef enum {
    FONT_SIZE_XS    = 12,
    FONT_SIZE_SM    = 14,
    FONT_SIZE_BASE  = 16,
    FONT_SIZE_LG    = 18,
    FONT_SIZE_XL    = 20,
    FONT_SIZE_2XL   = 24,
    FONT_SIZE_3XL   = 30,
    FONT_SIZE_4XL   = 36,
    FONT_SIZE_5XL   = 48,
    FONT_SIZE_6XL   = 60,
    FONT_SIZE_7XL   = 72,
    FONT_SIZE_8XL   = 96,
    FONT_SIZE_9XL   = 128,
} ui_font_size_t;

typedef enum {
    FONT_WEIGHT_THIN       = 100,
    FONT_WEIGHT_EXTRALIGHT = 200,
    FONT_WEIGHT_LIGHT      = 300,
    FONT_WEIGHT_NORMAL     = 400,
    FONT_WEIGHT_MEDIUM     = 500,
    FONT_WEIGHT_SEMIBOLD   = 600,
    FONT_WEIGHT_BOLD       = 700,
    FONT_WEIGHT_EXTRABOLD  = 800,
    FONT_WEIGHT_BLACK      = 900,
} ui_font_weight_t;

typedef enum {
    LINE_HEIGHT_NONE    = 1,
    LINE_HEIGHT_TIGHT   = 2,   /* 1.25 */
    LINE_HEIGHT_SNUG    = 3,   /* 1.375 */
    LINE_HEIGHT_NORMAL  = 4,   /* 1.5 */
    LINE_HEIGHT_RELAXED = 5,   /* 1.625 */
    LINE_HEIGHT_LOOSE   = 6,   /* 2 */
} ui_line_height_t;

typedef enum {
    LETTER_SPACING_TIGHTER = 0, /* -0.05em */
    LETTER_SPACING_TIGHT   = 1, /* -0.025em */
    LETTER_SPACING_NORMAL  = 2, /* 0 */
    LETTER_SPACING_WIDE    = 3, /* 0.025em */
    LETTER_SPACING_WIDER   = 4, /* 0.05em */
    LETTER_SPACING_WIDEST  = 5, /* 0.1em */
} ui_letter_spacing_t;

typedef enum {
    FONT_FAMILY_SANS        = 0,
    FONT_FAMILY_SERIF       = 1,
    FONT_FAMILY_MONO        = 2,
    FONT_FAMILY_DISPLAY     = 3,
    FONT_FAMILY_HANDWRITING = 4,
} ui_font_family_t;

typedef struct {
    const char        *name;
    ui_font_size_t     size;
    ui_font_weight_t   weight;
    ui_line_height_t   line_height;
    ui_letter_spacing_t letter_spacing;
    ui_font_family_t   family;
    const char        *family_custom;  /* e.g. "Inter, sans-serif" */
    ui_token_layer_t   layer;
} ui_typography_token_t;

/* ===== Border Radius ===== */
typedef enum {
    RADIUS_NONE = 0,
    RADIUS_SM   = 2,   /* 0.125rem */
    RADIUS_BASE = 4,   /* 0.25rem */
    RADIUS_MD   = 6,   /* 0.375rem */
    RADIUS_LG   = 8,   /* 0.5rem */
    RADIUS_XL   = 12,  /* 0.75rem */
    RADIUS_2XL  = 16,  /* 1rem */
    RADIUS_3XL  = 24,  /* 1.5rem */
    RADIUS_FULL = 9999,/* 9999px */
} ui_border_radius_t;

/* ===== Shadow Levels ===== */
typedef enum {
    SHADOW_NONE = 0,
    SHADOW_SM   = 1,
    SHADOW_BASE = 2,
    SHADOW_MD   = 3,
    SHADOW_LG   = 4,
    SHADOW_XL   = 5,
    SHADOW_2XL  = 6,
    SHADOW_INNER = 7,
} ui_shadow_level_t;

typedef struct {
    const char       *name;
    ui_shadow_level_t level;
    int               offset_x;
    int               offset_y;
    int               blur;
    int               spread;
    ui_color_id_t     color_id;
    float             opacity;    /* 0.0 - 1.0 */
    bool              inset;
} ui_shadow_token_t;

/* ===== Animation Tokens ===== */
typedef enum {
    EASE_LINEAR       = 0,
    EASE_IN           = 1,
    EASE_OUT          = 2,
    EASE_IN_OUT       = 3,
    EASE_SPRING       = 4,
    EASE_BOUNCE       = 5,
} ui_easing_t;

typedef struct {
    const char   *name;
    ui_easing_t   easing;
    int           duration_ms;
    int           delay_ms;
    const char   *property;  /* CSS property or "all" */
} ui_animation_token_t;

/* ===== Breakpoints ===== */
typedef enum {
    BREAKPOINT_SM  = 640,
    BREAKPOINT_MD  = 768,
    BREAKPOINT_LG  = 1024,
    BREAKPOINT_XL  = 1280,
    BREAKPOINT_2XL = 1536,
} ui_breakpoint_t;

/* ===== Design Token ===== */
typedef struct ui_design_token_s {
    const char           *name;
    const char           *css_variable;  /* e.g. "--color-primary" */
    const char           *value;         /* CSS value string */
    ui_token_layer_t      layer;
    const char           *inherits_from; /* parent token name or NULL */
    bool                  is_dark_override;
    const char           *dark_value;    /* alternative for dark mode */
    struct ui_design_token_s **aliases;
    int                   alias_count;
    int                   alias_capacity;
} ui_design_token_t;

/* ===== Token Collection / Theme ===== */
#define UI_MAX_TOKENS 512
#define UI_MAX_CSS_VAR_LEN 64
#define UI_MAX_VALUE_LEN 128

typedef struct {
    ui_design_token_t **tokens;
    int                 count;
    int                 capacity;
    const char         *name;
    bool                is_dark;
} ui_token_collection_t;

/* ===== Color Palette Definition ===== */
typedef struct {
    ui_color_id_t id;
    const char   *name;
    uint8_t       r, g, b;
} ui_palette_entry_t;

/* ===== API ===== */

/* Initialize the global token system */
void ui_tokens_init(void);

/* Shutdown and free resources */
void ui_tokens_shutdown(void);

/* Create a new design token */
ui_design_token_t* ui_token_create(
    const char      *name,
    ui_token_layer_t layer,
    const char      *css_variable,
    const char      *value);

/* Add a dark mode override to an existing token */
void ui_token_set_dark_override(
    ui_design_token_t *token,
    const char        *dark_value);

/* Establish token inheritance (parent->child alias chain) */
void ui_token_set_inherits(
    ui_design_token_t *child,
    const char        *parent_name);

/* Add an alias reference */
void ui_token_add_alias(
    ui_design_token_t *token,
    ui_design_token_t *alias);

/* Lookup a token by name */
ui_design_token_t* ui_token_find(const char *name);

/* Lookup a token by CSS variable name */
ui_design_token_t* ui_token_find_by_var(const char *css_var);

/* Generate CSS custom properties string from tokens */
int ui_tokens_to_css(
    const ui_token_collection_t *coll,
    bool                         dark_mode,
    char                        *buffer,
    int                          buffer_size);

/* Generate CSS for a specific token layer */
int ui_tokens_layer_to_css(
    ui_token_layer_t layer,
    bool             dark_mode,
    char            *buffer,
    int              buffer_size);

/* Get palette color by ID */
const ui_palette_entry_t* ui_palette_get(ui_color_id_t id);

/* Get the full palette (returns array, *count set to size) */
const ui_palette_entry_t* ui_palette_all(int *count);

/* Convert a spacing value to a CSS rem string */
const char* ui_spacing_to_rem(ui_spacing_t space);

/* Convert a spacing value to px */
int ui_spacing_to_px(ui_spacing_t space);

/* Typography token to CSS string */
int ui_typography_to_css(
    const ui_typography_token_t *typo,
    char *buffer, int buf_size);

/* Shadow token to CSS box-shadow string */
int ui_shadow_to_css(
    const ui_shadow_token_t *shadow,
    const ui_palette_entry_t *color,
    char *buffer, int buf_size);

/* Animation token to CSS transition string */
int ui_animation_to_css(
    const ui_animation_token_t *anim,
    char *buffer, int buf_size);

/* Create a new token collection */
ui_token_collection_t* ui_token_collection_create(const char *name);

/* Add token to collection */
void ui_token_collection_add(
    ui_token_collection_t *coll,
    ui_design_token_t     *token);

/* Free a token collection */
void ui_token_collection_free(ui_token_collection_t *coll);

/* Merge two collections (dest gets tokens from src) */
void ui_token_collection_merge(
    ui_token_collection_t *dest,
    const ui_token_collection_t *src);

/* Get default global token set (light mode) */
const ui_token_collection_t* ui_tokens_global_light(void);

/* Get default global token set (dark mode) */
const ui_token_collection_t* ui_tokens_global_dark(void);

#endif /* MINI_UI_DESIGN_TOKENS_H */
