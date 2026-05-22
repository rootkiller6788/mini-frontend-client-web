#include "design_tokens.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define UI_GLOBAL_COLOR_COUNT (COLOR_COUNT)
#define UI_PALETTE_ENTRIES    104

static const ui_palette_entry_t ui_builtin_palette[] = {
    {COLOR_SLATE_50,   "slate-50",   0xF8, 0xFA, 0xFC},
    {COLOR_SLATE_100,  "slate-100",  0xF1, 0xF5, 0xF9},
    {COLOR_SLATE_200,  "slate-200",  0xE2, 0xE8, 0xF0},
    {COLOR_SLATE_300,  "slate-300",  0xCB, 0xD5, 0xE1},
    {COLOR_SLATE_400,  "slate-400",  0x94, 0xA3, 0xB8},
    {COLOR_SLATE_500,  "slate-500",  0x64, 0x74, 0x8B},
    {COLOR_SLATE_600,  "slate-600",  0x47, 0x55, 0x69},
    {COLOR_SLATE_700,  "slate-700",  0x33, 0x41, 0x55},
    {COLOR_SLATE_800,  "slate-800",  0x1E, 0x29, 0x3B},
    {COLOR_SLATE_900,  "slate-900",  0x0F, 0x17, 0x2A},
    {COLOR_SLATE_950,  "slate-950",  0x02, 0x06, 0x17},
    {COLOR_GRAY_50,    "gray-50",    0xF9, 0xFA, 0xFB},
    {COLOR_GRAY_100,   "gray-100",   0xF3, 0xF4, 0xF6},
    {COLOR_GRAY_200,   "gray-200",   0xE5, 0xE7, 0xEB},
    {COLOR_GRAY_300,   "gray-300",   0xD1, 0xD5, 0xDB},
    {COLOR_GRAY_400,   "gray-400",   0x9C, 0xA3, 0xAF},
    {COLOR_GRAY_500,   "gray-500",   0x6B, 0x72, 0x80},
    {COLOR_GRAY_600,   "gray-600",   0x4B, 0x55, 0x63},
    {COLOR_GRAY_700,   "gray-700",   0x37, 0x41, 0x51},
    {COLOR_GRAY_800,   "gray-800",   0x1F, 0x29, 0x37},
    {COLOR_GRAY_900,   "gray-900",   0x11, 0x18, 0x27},
    {COLOR_GRAY_950,   "gray-950",   0x03, 0x07, 0x12},
    {COLOR_RED_50,     "red-50",     0xFE, 0xF2, 0xF2},
    {COLOR_RED_100,    "red-100",    0xFE, 0xE2, 0xE2},
    {COLOR_RED_200,    "red-200",    0xFE, 0xCA, 0xCA},
    {COLOR_RED_300,    "red-300",    0xFC, 0xA5, 0xA5},
    {COLOR_RED_400,    "red-400",    0xF8, 0x71, 0x71},
    {COLOR_RED_500,    "red-500",    0xEF, 0x44, 0x44},
    {COLOR_RED_600,    "red-600",    0xDC, 0x26, 0x26},
    {COLOR_RED_700,    "red-700",    0xB9, 0x1C, 0x1C},
    {COLOR_RED_800,    "red-800",    0x99, 0x1B, 0x1B},
    {COLOR_RED_900,    "red-900",    0x7F, 0x1D, 0x1D},
    {COLOR_RED_950,    "red-950",    0x45, 0x0A, 0x0A},
    {COLOR_BLUE_50,    "blue-50",    0xEF, 0xF6, 0xFF},
    {COLOR_BLUE_100,   "blue-100",   0xDB, 0xEA, 0xFE},
    {COLOR_BLUE_200,   "blue-200",   0xBF, 0xDB, 0xFE},
    {COLOR_BLUE_300,   "blue-300",   0x93, 0xC5, 0xFD},
    {COLOR_BLUE_400,   "blue-400",   0x60, 0xA5, 0xFA},
    {COLOR_BLUE_500,   "blue-500",   0x3B, 0x82, 0xF6},
    {COLOR_BLUE_600,   "blue-600",   0x25, 0x63, 0xEB},
    {COLOR_BLUE_700,   "blue-700",   0x1D, 0x4E, 0xD8},
    {COLOR_BLUE_800,   "blue-800",   0x1E, 0x40, 0xAF},
    {COLOR_BLUE_900,   "blue-900",   0x1E, 0x3A, 0x8A},
    {COLOR_BLUE_950,   "blue-950",   0x17, 0x22, 0x54},
    {COLOR_GREEN_50,   "green-50",   0xF0, 0xFD, 0xF4},
    {COLOR_GREEN_100,  "green-100",  0xDC, 0xFC, 0xE7},
    {COLOR_GREEN_200,  "green-200",  0xBB, 0xF7, 0xD0},
    {COLOR_GREEN_300,  "green-300",  0x86, 0xEF, 0xAC},
    {COLOR_GREEN_400,  "green-400",  0x4A, 0xDE, 0x80},
    {COLOR_GREEN_500,  "green-500",  0x22, 0xC5, 0x5E},
    {COLOR_GREEN_600,  "green-600",  0x16, 0xA3, 0x4A},
    {COLOR_GREEN_700,  "green-700",  0x15, 0x80, 0x3D},
    {COLOR_GREEN_800,  "green-800",  0x16, 0x6D, 0x34},
    {COLOR_GREEN_900,  "green-900",  0x14, 0x53, 0x2D},
    {COLOR_GREEN_950,  "green-950",  0x05, 0x2E, 0x16},
    {COLOR_YELLOW_50,  "yellow-50",  0xFE, 0xFC, 0xE8},
    {COLOR_YELLOW_100, "yellow-100", 0xFE, 0xF9, 0xC3},
    {COLOR_YELLOW_200, "yellow-200", 0xFE, 0xF0, 0x8A},
    {COLOR_YELLOW_300, "yellow-300", 0xFD, 0xDF, 0x49},
    {COLOR_YELLOW_400, "yellow-400", 0xFA, 0xCC, 0x15},
    {COLOR_YELLOW_500, "yellow-500", 0xEA, 0xAB, 0x0C},
    {COLOR_YELLOW_600, "yellow-600", 0xCA, 0x8A, 0x04},
    {COLOR_YELLOW_700, "yellow-700", 0xA1, 0x62, 0x07},
    {COLOR_YELLOW_800, "yellow-800", 0x85, 0x4D, 0x0E},
    {COLOR_YELLOW_900, "yellow-900", 0x71, 0x3F, 0x12},
    {COLOR_YELLOW_950, "yellow-950", 0x42, 0x22, 0x06},
    {COLOR_PURPLE_50,  "purple-50",  0xFA, 0xF5, 0xFF},
    {COLOR_PURPLE_100, "purple-100", 0xF3, 0xE8, 0xFF},
    {COLOR_PURPLE_200, "purple-200", 0xE9, 0xD5, 0xFF},
    {COLOR_PURPLE_300, "purple-300", 0xD8, 0xB4, 0xFE},
    {COLOR_PURPLE_400, "purple-400", 0xC0, 0x84, 0xFC},
    {COLOR_PURPLE_500, "purple-500", 0xA8, 0x55, 0xF7},
    {COLOR_PURPLE_600, "purple-600", 0x93, 0x33, 0xEA},
    {COLOR_PURPLE_700, "purple-700", 0x7E, 0x22, 0xCE},
    {COLOR_PURPLE_800, "purple-800", 0x6B, 0x21, 0xA8},
    {COLOR_PURPLE_900, "purple-900", 0x58, 0x1C, 0x87},
    {COLOR_PURPLE_950, "purple-950", 0x3B, 0x07, 0x64},
    {COLOR_WHITE,      "white",      0xFF, 0xFF, 0xFF},
    {COLOR_BLACK,      "black",      0x00, 0x00, 0x00},
    {COLOR_TRANSPARENT,"transparent", 0x00, 0x00, 0x00},
};

static ui_token_collection_t *g_global_light = NULL;
static ui_token_collection_t *g_global_dark  = NULL;
static ui_design_token_t    **g_all_tokens   = NULL;
static int                    g_token_count  = 0;
static int                    g_token_cap    = 0;

static void ensure_token_registry(int needed) {
    if (g_token_cap >= needed) return;
    int new_cap = g_token_cap ? g_token_cap * 2 : 128;
    if (new_cap < needed) new_cap = needed;
    ui_design_token_t **new_arr = (ui_design_token_t**)realloc(
        g_all_tokens, (size_t)new_cap * sizeof(ui_design_token_t*));
    if (!new_arr) return;
    g_all_tokens = new_arr;
    g_token_cap  = new_cap;
}

void ui_tokens_init(void) {
    ensure_token_registry(256);
    g_global_light = ui_token_collection_create("global-light");
    g_global_dark  = ui_token_collection_create("global-dark");

    ui_design_token_t *t;

    /* Colors - Global Layer */
    t = ui_token_create("color-slate-50",   TOKEN_LAYER_GLOBAL, "--color-slate-50",   "#f8fafc"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("color-slate-900",  TOKEN_LAYER_GLOBAL, "--color-slate-900",  "#0f172a"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("color-white",      TOKEN_LAYER_GLOBAL, "--color-white",      "#ffffff"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("color-black",      TOKEN_LAYER_GLOBAL, "--color-black",      "#000000"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("color-blue-500",   TOKEN_LAYER_GLOBAL, "--color-blue-500",   "#3b82f6"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("color-blue-600",   TOKEN_LAYER_GLOBAL, "--color-blue-600",   "#2563eb"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("color-blue-700",   TOKEN_LAYER_GLOBAL, "--color-blue-700",   "#1d4ed8"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("color-red-500",    TOKEN_LAYER_GLOBAL, "--color-red-500",    "#ef4444"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("color-red-600",    TOKEN_LAYER_GLOBAL, "--color-red-600",    "#dc2626"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("color-green-500",  TOKEN_LAYER_GLOBAL, "--color-green-500",  "#22c55e"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("color-yellow-500", TOKEN_LAYER_GLOBAL, "--color-yellow-500", "#eab308"); ui_token_collection_add(g_global_light, t);

    /* Alias Layer */
    t = ui_token_create("color-bg-primary",    TOKEN_LAYER_ALIAS, "--color-bg-primary",    "var(--color-white)");     ui_token_collection_add(g_global_light, t);
    t = ui_token_create("color-bg-secondary",  TOKEN_LAYER_ALIAS, "--color-bg-secondary",  "var(--color-slate-50)");  ui_token_collection_add(g_global_light, t);
    t = ui_token_create("color-text-primary",  TOKEN_LAYER_ALIAS, "--color-text-primary",  "var(--color-slate-900)"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("color-text-secondary",TOKEN_LAYER_ALIAS, "--color-text-secondary","#64748b");                ui_token_collection_add(g_global_light, t);
    t = ui_token_create("color-border",        TOKEN_LAYER_ALIAS, "--color-border",        "var(--color-slate-200)"); ui_token_collection_add(g_global_light, t);

    /* Component Layer */
    t = ui_token_create("btn-bg-primary",      TOKEN_LAYER_COMPONENT, "--btn-bg-primary",      "var(--color-blue-500)");  ui_token_collection_add(g_global_light, t);
    t = ui_token_create("btn-bg-primary-hover",TOKEN_LAYER_COMPONENT, "--btn-bg-primary-hover","var(--color-blue-600)");  ui_token_collection_add(g_global_light, t);
    t = ui_token_create("btn-text-primary",    TOKEN_LAYER_COMPONENT, "--btn-text-primary",    "var(--color-white)");      ui_token_collection_add(g_global_light, t);

    /* Spacing Tokens */
    t = ui_token_create("space-1",  TOKEN_LAYER_GLOBAL, "--space-1",  "0.25rem"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("space-2",  TOKEN_LAYER_GLOBAL, "--space-2",  "0.5rem");  ui_token_collection_add(g_global_light, t);
    t = ui_token_create("space-4",  TOKEN_LAYER_GLOBAL, "--space-4",  "1rem");    ui_token_collection_add(g_global_light, t);
    t = ui_token_create("space-8",  TOKEN_LAYER_GLOBAL, "--space-8",  "2rem");    ui_token_collection_add(g_global_light, t);

    /* Typography Tokens */
    t = ui_token_create("font-size-base",  TOKEN_LAYER_GLOBAL, "--font-size-base",  "1rem");    ui_token_collection_add(g_global_light, t);
    t = ui_token_create("font-size-lg",    TOKEN_LAYER_GLOBAL, "--font-size-lg",    "1.125rem"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("font-size-xl",    TOKEN_LAYER_GLOBAL, "--font-size-xl",    "1.25rem");  ui_token_collection_add(g_global_light, t);

    /* Border Radius */
    t = ui_token_create("radius-sm",  TOKEN_LAYER_GLOBAL, "--radius-sm",  "0.125rem"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("radius-md",  TOKEN_LAYER_GLOBAL, "--radius-md",  "0.375rem"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("radius-lg",  TOKEN_LAYER_GLOBAL, "--radius-lg",  "0.5rem");   ui_token_collection_add(g_global_light, t);

    /* Shadow Tokens */
    t = ui_token_create("shadow-sm", TOKEN_LAYER_GLOBAL, "--shadow-sm", "0 1px 2px 0 rgb(0 0 0 / 0.05)"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("shadow-md", TOKEN_LAYER_GLOBAL, "--shadow-md", "0 4px 6px -1px rgb(0 0 0 / 0.1)"); ui_token_collection_add(g_global_light, t);
    t = ui_token_create("shadow-lg", TOKEN_LAYER_GLOBAL, "--shadow-lg", "0 10px 15px -3px rgb(0 0 0 / 0.1)"); ui_token_collection_add(g_global_light, t);

    /* Dark mode tokens */
    t = ui_token_create("color-bg-primary",   TOKEN_LAYER_ALIAS, "--color-bg-primary",   "#0f172a"); ui_token_collection_add(g_global_dark, t);
    t = ui_token_create("color-bg-secondary", TOKEN_LAYER_ALIAS, "--color-bg-secondary", "#1e293b"); ui_token_collection_add(g_global_dark, t);
    t = ui_token_create("color-text-primary", TOKEN_LAYER_ALIAS, "--color-text-primary", "#f1f5f9"); ui_token_collection_add(g_global_dark, t);
    t = ui_token_create("color-text-secondary",TOKEN_LAYER_ALIAS,"--color-text-secondary","#94a3b8");ui_token_collection_add(g_global_dark, t);
    t = ui_token_create("color-border",       TOKEN_LAYER_ALIAS, "--color-border",       "#334155"); ui_token_collection_add(g_global_dark, t);
    t = ui_token_create("btn-bg-primary",     TOKEN_LAYER_COMPONENT, "--btn-bg-primary", "var(--color-blue-500)"); ui_token_collection_add(g_global_dark, t);
}

void ui_tokens_shutdown(void) {
    if (g_global_light) { ui_token_collection_free(g_global_light); g_global_light = NULL; }
    if (g_global_dark)  { ui_token_collection_free(g_global_dark);  g_global_dark  = NULL; }
    if (g_all_tokens) {
        for (int i = 0; i < g_token_count; i++) {
            if (g_all_tokens[i]) {
                free(g_all_tokens[i]->aliases);
                free(g_all_tokens[i]);
            }
        }
        free(g_all_tokens);
        g_all_tokens = NULL;
    }
    g_token_count = 0;
    g_token_cap   = 0;
}

ui_design_token_t* ui_token_create(
    const char *name, ui_token_layer_t layer,
    const char *css_variable, const char *value)
{
    ui_design_token_t *t = (ui_design_token_t*)calloc(1, sizeof(ui_design_token_t));
    if (!t) return NULL;
    t->name         = name ? _strdup(name) : NULL;
    t->layer        = layer;
    t->css_variable = css_variable ? _strdup(css_variable) : NULL;
    t->value        = value ? _strdup(value) : NULL;
    t->dark_value   = NULL;
    t->inherits_from = NULL;
    t->is_dark_override = false;
    t->aliases      = NULL;
    t->alias_count  = 0;
    t->alias_capacity = 0;

    ensure_token_registry(g_token_count + 1);
    g_all_tokens[g_token_count++] = t;
    return t;
}

void ui_token_set_dark_override(ui_design_token_t *token, const char *dark_value) {
    if (!token) return;
    if (token->dark_value) free((void*)token->dark_value);
    token->dark_value = dark_value ? _strdup(dark_value) : NULL;
    token->is_dark_override = (dark_value != NULL);
}

void ui_token_set_inherits(ui_design_token_t *child, const char *parent_name) {
    if (!child) return;
    if (child->inherits_from) free((void*)child->inherits_from);
    child->inherits_from = parent_name ? _strdup(parent_name) : NULL;
}

void ui_token_add_alias(ui_design_token_t *token, ui_design_token_t *alias) {
    if (!token || !alias) return;
    if (token->alias_count >= token->alias_capacity) {
        int nc = token->alias_capacity ? token->alias_capacity * 2 : 4;
        ui_design_token_t **na = (ui_design_token_t**)realloc(
            token->aliases, (size_t)nc * sizeof(ui_design_token_t*));
        if (!na) return;
        token->aliases = na;
        token->alias_capacity = nc;
    }
    token->aliases[token->alias_count++] = alias;
}

ui_design_token_t* ui_token_find(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < g_token_count; i++) {
        if (g_all_tokens[i] && g_all_tokens[i]->name &&
            strcmp(g_all_tokens[i]->name, name) == 0)
            return g_all_tokens[i];
    }
    return NULL;
}

ui_design_token_t* ui_token_find_by_var(const char *css_var) {
    if (!css_var) return NULL;
    for (int i = 0; i < g_token_count; i++) {
        if (g_all_tokens[i] && g_all_tokens[i]->css_variable &&
            strcmp(g_all_tokens[i]->css_variable, css_var) == 0)
            return g_all_tokens[i];
    }
    return NULL;
}

int ui_tokens_to_css(const ui_token_collection_t *coll, bool dark_mode,
                     char *buffer, int buffer_size) {
    if (!coll || !buffer || buffer_size <= 0) return 0;
    int pos = 0;
    pos += snprintf(buffer + pos, (size_t)(buffer_size - pos), "  /* %s */\n",
                    dark_mode ? "Dark Mode Theme" : "Light Mode Theme");
    for (int i = 0; i < coll->count; i++) {
        ui_design_token_t *t = coll->tokens[i];
        if (!t || !t->css_variable) continue;
        const char *val = (dark_mode && t->dark_value) ? t->dark_value : t->value;
        if (!val) continue;
        pos += snprintf(buffer + pos, (size_t)(buffer_size - pos),
                        "  %s: %s;\n", t->css_variable, val);
        if (pos >= buffer_size) break;
    }
    return pos;
}

int ui_tokens_layer_to_css(ui_token_layer_t layer, bool dark_mode,
                           char *buffer, int buffer_size) {
    if (!buffer || buffer_size <= 0) return 0;
    int pos = 0;
    const char *layer_names[] = {"Global", "Alias", "Component"};
    pos += snprintf(buffer + pos, (size_t)(buffer_size - pos),
                    "  /* %s Layer */\n",
                    layer <= TOKEN_LAYER_COMPONENT ? layer_names[layer] : "Unknown");
    for (int i = 0; i < g_token_count; i++) {
        ui_design_token_t *t = g_all_tokens[i];
        if (!t || t->layer != layer || !t->css_variable) continue;
        const char *val = (dark_mode && t->dark_value) ? t->dark_value : t->value;
        if (!val) continue;
        pos += snprintf(buffer + pos, (size_t)(buffer_size - pos),
                        "  %s: %s;\n", t->css_variable, val);
        if (pos >= buffer_size) break;
    }
    return pos;
}

const ui_palette_entry_t* ui_palette_get(ui_color_id_t id) {
    if (id >= COLOR_COUNT) return NULL;
    return &ui_builtin_palette[id];
}

const ui_palette_entry_t* ui_palette_all(int *count) {
    if (count) *count = (int)(sizeof(ui_builtin_palette) / sizeof(ui_builtin_palette[0]));
    return ui_builtin_palette;
}

const char* ui_spacing_to_rem(ui_spacing_t space) {
    static char buf[16];
    snprintf(buf, sizeof(buf), "%.3frem", (float)space / 16.0f);
    return buf;
}

int ui_spacing_to_px(ui_spacing_t space) {
    return (int)space;
}

int ui_typography_to_css(const ui_typography_token_t *typo,
                         char *buffer, int buf_size) {
    if (!typo || !buffer || buf_size <= 0) return 0;
    static const char *family_map[] = {
        "ui-sans-serif, system-ui, sans-serif",
        "ui-serif, Georgia, serif",
        "ui-monospace, monospace",
        "\"Inter\", system-ui, sans-serif",
        "\"Caveat\", cursive"
    };
    static const float lh_map[] = {1.0f, 1.25f, 1.375f, 1.5f, 1.625f, 2.0f};
    static const char *ls_map[] = {"-0.05em", "-0.025em", "0", "0.025em", "0.05em", "0.1em"};
    const char *family = typo->family_custom ? typo->family_custom :
        (typo->family <= FONT_FAMILY_HANDWRITING ? family_map[typo->family] : family_map[0]);
    return snprintf(buffer, (size_t)buf_size,
        "font-size: %dpx; font-weight: %d; line-height: %.3f; "
        "letter-spacing: %s; font-family: %s;",
        (int)typo->size, (int)typo->weight,
        lh_map[typo->line_height <= LINE_HEIGHT_LOOSE ? typo->line_height : LINE_HEIGHT_NORMAL],
        ls_map[typo->letter_spacing <= LETTER_SPACING_WIDEST ? typo->letter_spacing : 2],
        family);
}

int ui_shadow_to_css(const ui_shadow_token_t *shadow,
                     const ui_palette_entry_t *color,
                     char *buffer, int buf_size) {
    if (!shadow || !buffer || buf_size <= 0) return 0;
    const char *inset = shadow->inset ? "inset " : "";
    if (color) {
        return snprintf(buffer, (size_t)buf_size,
            "%s%dpx %dpx %dpx %dpx rgba(%d,%d,%d,%.2f)",
            inset, shadow->offset_x, shadow->offset_y,
            shadow->blur, shadow->spread,
            color->r, color->g, color->b, shadow->opacity);
    }
    return snprintf(buffer, (size_t)buf_size,
        "%s%dpx %dpx %dpx %dpx rgba(0,0,0,%.2f)",
        inset, shadow->offset_x, shadow->offset_y,
        shadow->blur, shadow->spread, shadow->opacity);
}

int ui_animation_to_css(const ui_animation_token_t *anim,
                        char *buffer, int buf_size) {
    if (!anim || !buffer || buf_size <= 0) return 0;
    static const char *ease_map[] = {
        "linear", "ease-in", "ease-out", "ease-in-out",
        "cubic-bezier(0.34, 1.56, 0.64, 1)",
        "cubic-bezier(0.68, -0.55, 0.27, 1.55)"
    };
    return snprintf(buffer, (size_t)buf_size,
        "transition: %s %dms %s %dms;",
        anim->property ? anim->property : "all",
        anim->duration_ms,
        ease_map[anim->easing <= EASE_BOUNCE ? anim->easing : 0],
        anim->delay_ms);
}

ui_token_collection_t* ui_token_collection_create(const char *name) {
    ui_token_collection_t *c = (ui_token_collection_t*)calloc(1, sizeof(ui_token_collection_t));
    if (!c) return NULL;
    c->name = name ? _strdup(name) : NULL;
    c->capacity = 64;
    c->count = 0;
    c->is_dark = false;
    c->tokens = (ui_design_token_t**)calloc((size_t)c->capacity, sizeof(ui_design_token_t*));
    return c;
}

void ui_token_collection_add(ui_token_collection_t *coll, ui_design_token_t *token) {
    if (!coll || !token) return;
    if (coll->count >= coll->capacity) {
        int nc = coll->capacity * 2;
        ui_design_token_t **nt = (ui_design_token_t**)realloc(
            coll->tokens, (size_t)nc * sizeof(ui_design_token_t*));
        if (!nt) return;
        coll->tokens = nt;
        coll->capacity = nc;
    }
    coll->tokens[coll->count++] = token;
}

void ui_token_collection_free(ui_token_collection_t *coll) {
    if (!coll) return;
    free(coll->tokens);
    free((void*)coll->name);
    free(coll);
}

void ui_token_collection_merge(ui_token_collection_t *dest,
                               const ui_token_collection_t *src) {
    if (!dest || !src) return;
    for (int i = 0; i < src->count; i++) {
        ui_token_collection_add(dest, src->tokens[i]);
    }
}

const ui_token_collection_t* ui_tokens_global_light(void) { return g_global_light; }
const ui_token_collection_t* ui_tokens_global_dark(void)  { return g_global_dark; }
