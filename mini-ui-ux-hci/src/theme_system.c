#include "theme_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static ui_theme_manager_t *g_theme_manager = NULL;
static ui_theme_t         *g_default_light  = NULL;
static ui_theme_t         *g_default_dark   = NULL;

static int ensure_theme_capacity(ui_theme_manager_t *mgr, int needed) {
    if (mgr->theme_capacity >= needed) return 0;
    int nc = mgr->theme_capacity ? mgr->theme_capacity * 2 : 16;
    if (nc < needed) nc = needed;
    ui_theme_t **nt = (ui_theme_t**)realloc(
        mgr->themes, (size_t)nc * sizeof(ui_theme_t*));
    if (!nt) return -1;
    mgr->themes = nt;
    mgr->theme_capacity = nc;
    return 0;
}

static int ensure_var_capacity(ui_theme_t *theme, int needed) {
    if (theme->var_capacity >= needed) return 0;
    int nc = theme->var_capacity ? theme->var_capacity * 2 : 64;
    if (nc < needed) nc = needed;
    ui_theme_variable_t *nv = (ui_theme_variable_t*)realloc(
        theme->variables, (size_t)nc * sizeof(ui_theme_variable_t));
    if (!nv) return -1;
    theme->variables = nv;
    theme->var_capacity = nc;
    return 0;
}

void ui_theme_init(void) {
    ui_theme_provider_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.strategy = THEME_PROVIDER_STRATEGY_CSS_VARS;
    cfg.attribute_name = "data-theme";
    cfg.class_name = "dark";
    cfg.use_local_storage = true;
    cfg.storage_key = "mini-ui-theme";
    ui_theme_init_with_config(&cfg);
}

void ui_theme_init_with_config(const ui_theme_provider_config_t *config) {
    g_theme_manager = (ui_theme_manager_t*)calloc(1, sizeof(ui_theme_manager_t));
    if (!g_theme_manager) return;
    g_theme_manager->theme_capacity = 16;
    g_theme_manager->themes = (ui_theme_t**)calloc(16, sizeof(ui_theme_t*));
    g_theme_manager->override_capacity = 8;
    g_theme_manager->active_overrides = (ui_theme_override_t*)calloc(8, sizeof(ui_theme_override_t));
    if (config) {
        g_theme_manager->root_context = ui_theme_context_create(THEME_MODE_AUTO);
        g_theme_manager->root_context->provider_config = *config;
    } else {
        g_theme_manager->root_context = ui_theme_context_create(THEME_MODE_AUTO);
    }
    g_theme_manager->root_context->transition_duration_ms = 300;
    g_theme_manager->root_context->transition_easing = "ease-in-out";

    g_default_light = ui_theme_create_default_light();
    g_default_dark  = ui_theme_create_default_dark();
    ui_theme_register(g_default_light);
    ui_theme_register(g_default_dark);

    g_theme_manager->system_prefs = ui_theme_detect_system_prefs();
    g_theme_manager->initialized = true;
}

void ui_theme_shutdown(void) {
    if (!g_theme_manager) return;
    for (int i = 0; i < g_theme_manager->theme_count; i++) {
        ui_theme_free(g_theme_manager->themes[i]);
    }
    free(g_theme_manager->themes);
    free(g_theme_manager->active_overrides);
    if (g_theme_manager->root_context) {
        ui_theme_context_free(g_theme_manager->root_context);
    }
    free(g_theme_manager);
    g_theme_manager = NULL;
    g_default_light = NULL;
    g_default_dark  = NULL;
}

ui_system_preferences_t ui_theme_detect_system_prefs(void) {
    ui_system_preferences_t prefs;
    memset(&prefs, 0, sizeof(prefs));
    prefs.supports_prefers_color_scheme = true;
    prefs.supports_color_scheme_meta = true;
    return prefs;
}

ui_color_scheme_t ui_theme_get_color_scheme(void) {
    return COLOR_SCHEME_LIGHT_DARK;
}

bool ui_theme_system_prefers_dark(void) {
    return false;
}

ui_theme_manager_t* ui_theme_manager_get(void) {
    return g_theme_manager;
}

void ui_theme_register(ui_theme_t *theme) {
    if (!g_theme_manager || !theme) return;
    ensure_theme_capacity(g_theme_manager, g_theme_manager->theme_count + 1);
    g_theme_manager->themes[g_theme_manager->theme_count++] = theme;
}

void ui_theme_unregister(const char *theme_name) {
    if (!g_theme_manager || !theme_name) return;
    for (int i = 0; i < g_theme_manager->theme_count; i++) {
        if (g_theme_manager->themes[i] &&
            strcmp(g_theme_manager->themes[i]->name, theme_name) == 0) {
            ui_theme_free(g_theme_manager->themes[i]);
            for (int j = i; j < g_theme_manager->theme_count - 1; j++) {
                g_theme_manager->themes[j] = g_theme_manager->themes[j + 1];
            }
            g_theme_manager->theme_count--;
            return;
        }
    }
}

ui_theme_t* ui_theme_get(const char *name) {
    if (!g_theme_manager || !name) return NULL;
    for (int i = 0; i < g_theme_manager->theme_count; i++) {
        if (g_theme_manager->themes[i] &&
            strcmp(g_theme_manager->themes[i]->name, name) == 0)
            return g_theme_manager->themes[i];
    }
    return NULL;
}

int ui_theme_list_names(const char ***names) {
    static const char *name_ptrs[64];
    if (!g_theme_manager) { if (names) *names = NULL; return 0; }
    int n = g_theme_manager->theme_count;
    if (n > 64) n = 64;
    for (int i = 0; i < n; i++) {
        name_ptrs[i] = g_theme_manager->themes[i] ?
            g_theme_manager->themes[i]->name : "unknown";
    }
    if (names) *names = name_ptrs;
    return n;
}

ui_theme_context_t* ui_theme_context_create(ui_theme_mode_t mode) {
    ui_theme_context_t *ctx = (ui_theme_context_t*)calloc(1, sizeof(ui_theme_context_t));
    if (!ctx) return NULL;
    ctx->mode = mode;
    ctx->ref_count = 1;
    ctx->transition_duration_ms = 300;
    ctx->transition_easing = "ease-in-out";
    return ctx;
}

void ui_theme_context_set_theme(ui_theme_context_t *ctx, ui_theme_t *theme) {
    if (!ctx) return;
    ctx->active_theme = theme;
}

void ui_theme_context_light(ui_theme_context_t *ctx) {
    if (!ctx) return;
    ctx->mode = THEME_MODE_LIGHT;
    ctx->is_dark = false;
    ctx->active_theme = g_default_light;
}

void ui_theme_context_dark(ui_theme_context_t *ctx) {
    if (!ctx) return;
    ctx->mode = THEME_MODE_DARK;
    ctx->is_dark = true;
    ctx->active_theme = g_default_dark;
}

void ui_theme_context_toggle(ui_theme_context_t *ctx) {
    if (!ctx) return;
    if (ctx->is_dark) {
        ui_theme_context_light(ctx);
    } else {
        ui_theme_context_dark(ctx);
    }
}

ui_theme_context_t* ui_theme_context_create_child(ui_theme_context_t *parent) {
    if (!parent) return ui_theme_context_create(THEME_MODE_AUTO);
    ui_theme_context_t *child = ui_theme_context_create(parent->mode);
    child->parent = parent;
    child->active_theme = parent->active_theme;
    child->is_dark = parent->is_dark;
    child->provider_config = parent->provider_config;
    parent->ref_count++;
    return child;
}

ui_theme_context_t* ui_theme_context_root(void) {
    return g_theme_manager ? g_theme_manager->root_context : NULL;
}

ui_theme_mode_t ui_theme_context_effective_mode(const ui_theme_context_t *ctx) {
    if (!ctx) return THEME_MODE_AUTO;
    if (ctx->mode == THEME_MODE_AUTO) {
        return ui_theme_system_prefers_dark() ? THEME_MODE_DARK : THEME_MODE_LIGHT;
    }
    return ctx->mode;
}

bool ui_theme_context_is_dark(const ui_theme_context_t *ctx) {
    if (!ctx) return false;
    if (ctx->mode == THEME_MODE_DARK || ctx->mode == THEME_MODE_CUSTOM) return ctx->is_dark;
    return ctx->is_dark;
}

void ui_theme_context_free(ui_theme_context_t *ctx) {
    if (!ctx) return;
    ctx->ref_count--;
    if (ctx->ref_count <= 0) {
        free(ctx);
    }
}

ui_theme_variable_t* ui_theme_var_create(const char *name,
                                         const char *light_value,
                                         const char *dark_value) {
    ui_theme_variable_t *v = (ui_theme_variable_t*)calloc(1, sizeof(ui_theme_variable_t));
    if (!v) return NULL;
    v->name = name;
    v->light_value = light_value;
    v->dark_value = dark_value;
    return v;
}

void ui_theme_add_var(ui_theme_t *theme, ui_theme_variable_t *var) {
    if (!theme || !var) return;
    ensure_var_capacity(theme, theme->var_count + 1);
    theme->variables[theme->var_count++] = *var;
}

void ui_theme_remove_var(ui_theme_t *theme, const char *name) {
    if (!theme || !name) return;
    for (int i = 0; i < theme->var_count; i++) {
        if (theme->variables[i].name &&
            strcmp(theme->variables[i].name, name) == 0) {
            for (int j = i; j < theme->var_count - 1; j++) {
                theme->variables[j] = theme->variables[j + 1];
            }
            theme->var_count--;
            return;
        }
    }
}

const char* ui_theme_var_get(const ui_theme_t *theme, const char *var_name, bool is_dark) {
    if (!theme || !var_name) return NULL;
    for (int i = 0; i < theme->var_count; i++) {
        if (theme->variables[i].name &&
            strcmp(theme->variables[i].name, var_name) == 0) {
            return (is_dark && theme->variables[i].dark_value) ?
                theme->variables[i].dark_value : theme->variables[i].light_value;
        }
    }
    return NULL;
}

ui_theme_t* ui_theme_create(const char *name, ui_theme_mode_t mode) {
    ui_theme_t *t = (ui_theme_t*)calloc(1, sizeof(ui_theme_t));
    if (!t) return NULL;
    if (name) { strncpy(t->name, name, UI_THEME_NAME_MAX - 1); t->name[UI_THEME_NAME_MAX - 1] = '\0'; }
    t->mode = mode;
    t->var_capacity = 64;
    t->variables = (ui_theme_variable_t*)calloc(64, sizeof(ui_theme_variable_t));
    return t;
}

ui_theme_t* ui_theme_create_default_light(void) {
    ui_theme_t *t = ui_theme_create("default-light", THEME_MODE_LIGHT);
    if (!t) return NULL;
    t->is_default = true;
    t->color_scheme = COLOR_SCHEME_LIGHT;
    t->is_readonly = true;

    /* Core background/text colors */
    ui_theme_add_var(t, ui_theme_var_create("--color-bg-primary",    "#ffffff", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-bg-secondary",  "#f8fafc", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-bg-tertiary",   "#f1f5f9", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-text-primary",   "#0f172a", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-text-secondary", "#64748b", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-text-tertiary",  "#94a3b8", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-border",         "#e2e8f0", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-border-hover",   "#cbd5e1", NULL));

    /* Brand/Accent colors */
    ui_theme_add_var(t, ui_theme_var_create("--color-primary",        "#3b82f6", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-primary-hover",  "#2563eb", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-primary-active", "#1d4ed8", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-primary-text",   "#ffffff", NULL));

    /* Semantic colors */
    ui_theme_add_var(t, ui_theme_var_create("--color-success",        "#22c55e", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-warning",        "#eab308", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-error",          "#ef4444", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-info",           "#3b82f6", NULL));

    /* Shadows */
    ui_theme_add_var(t, ui_theme_var_create("--shadow-sm",  "0 1px 2px 0 rgb(0 0 0 / 0.05)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--shadow-md",  "0 4px 6px -1px rgb(0 0 0 / 0.1)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--shadow-lg",  "0 10px 15px -3px rgb(0 0 0 / 0.1)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--shadow-xl",  "0 20px 25px -5px rgb(0 0 0 / 0.1)", NULL));

    /* Spacing */
    ui_theme_add_var(t, ui_theme_var_create("--spacing-1", "0.25rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--spacing-2", "0.5rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--spacing-4", "1rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--spacing-6", "1.5rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--spacing-8", "2rem", NULL));

    /* Border Radius */
    ui_theme_add_var(t, ui_theme_var_create("--radius-sm",    "0.125rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--radius-md",    "0.375rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--radius-lg",    "0.5rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--radius-xl",    "0.75rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--radius-full",  "9999px", NULL));

    /* Typography */
    ui_theme_add_var(t, ui_theme_var_create("--font-sans",   "ui-sans-serif, system-ui, sans-serif", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--font-mono",   "ui-monospace, SFMono-Regular, monospace", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--font-size-sm",  "0.875rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--font-size-base","1rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--font-size-lg",  "1.125rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--font-size-xl",  "1.25rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--font-size-2xl", "1.5rem", NULL));

    /* Component tokens */
    ui_theme_add_var(t, ui_theme_var_create("--btn-bg-primary",      "var(--color-primary)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--btn-bg-primary-hover", "var(--color-primary-hover)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--btn-text-primary",    "var(--color-primary-text)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--input-border",        "var(--color-border)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--input-bg",            "var(--color-bg-primary)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--card-bg",             "var(--color-bg-primary)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--card-border",         "var(--color-border)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--modal-overlay",       "rgba(0, 0, 0, 0.5)", NULL));

    /* Animation */
    ui_theme_add_var(t, ui_theme_var_create("--transition-fast", "150ms ease", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--transition-base", "300ms ease-in-out", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--transition-slow", "500ms ease", NULL));

    /* Z-index */
    ui_theme_add_var(t, ui_theme_var_create("--z-dropdown", "1000", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--z-sticky",   "1020", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--z-modal",    "1050", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--z-toast",    "1100", NULL));

    return t;
}

ui_theme_t* ui_theme_create_default_dark(void) {
    ui_theme_t *t = ui_theme_create("default-dark", THEME_MODE_DARK);
    if (!t) return NULL;
    t->is_default = true;
    t->color_scheme = COLOR_SCHEME_DARK;
    t->is_readonly = true;

    ui_theme_add_var(t, ui_theme_var_create("--color-bg-primary",    "#0f172a", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-bg-secondary",  "#1e293b", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-bg-tertiary",   "#334155", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-text-primary",   "#f1f5f9", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-text-secondary", "#94a3b8", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-text-tertiary",  "#64748b", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-border",         "#334155", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-border-hover",   "#475569", NULL));

    ui_theme_add_var(t, ui_theme_var_create("--color-primary",        "#3b82f6", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-primary-hover",  "#60a5fa", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-primary-active", "#93c5fd", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-primary-text",   "#ffffff", NULL));

    ui_theme_add_var(t, ui_theme_var_create("--color-success",        "#22c55e", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-warning",        "#eab308", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-error",          "#ef4444", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--color-info",           "#3b82f6", NULL));

    ui_theme_add_var(t, ui_theme_var_create("--shadow-sm",  "0 1px 2px 0 rgb(0 0 0 / 0.3)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--shadow-md",  "0 4px 6px -1px rgb(0 0 0 / 0.4)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--shadow-lg",  "0 10px 15px -3px rgb(0 0 0 / 0.4)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--shadow-xl",  "0 20px 25px -5px rgb(0 0 0 / 0.4)", NULL));

    ui_theme_add_var(t, ui_theme_var_create("--spacing-1", "0.25rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--spacing-2", "0.5rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--spacing-4", "1rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--spacing-6", "1.5rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--spacing-8", "2rem", NULL));

    ui_theme_add_var(t, ui_theme_var_create("--radius-sm",    "0.125rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--radius-md",    "0.375rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--radius-lg",    "0.5rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--radius-xl",    "0.75rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--radius-full",  "9999px", NULL));

    ui_theme_add_var(t, ui_theme_var_create("--font-sans",   "ui-sans-serif, system-ui, sans-serif", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--font-mono",   "ui-monospace, SFMono-Regular, monospace", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--font-size-sm",  "0.875rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--font-size-base","1rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--font-size-lg",  "1.125rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--font-size-xl",  "1.25rem", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--font-size-2xl", "1.5rem", NULL));

    ui_theme_add_var(t, ui_theme_var_create("--btn-bg-primary",      "var(--color-primary)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--btn-bg-primary-hover", "var(--color-primary-hover)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--btn-text-primary",    "var(--color-primary-text)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--input-border",        "var(--color-border)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--input-bg",            "var(--color-bg-primary)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--card-bg",             "var(--color-bg-secondary)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--card-border",         "var(--color-border)", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--modal-overlay",       "rgba(0, 0, 0, 0.7)", NULL));

    ui_theme_add_var(t, ui_theme_var_create("--transition-fast", "150ms ease", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--transition-base", "300ms ease-in-out", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--transition-slow", "500ms ease", NULL));

    ui_theme_add_var(t, ui_theme_var_create("--z-dropdown", "1000", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--z-sticky",   "1020", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--z-modal",    "1050", NULL));
    ui_theme_add_var(t, ui_theme_var_create("--z-toast",    "1100", NULL));

    return t;
}

ui_theme_t* ui_theme_create_from_template(const ui_custom_theme_template_t *template_def) {
    if (!template_def) return NULL;
    ui_theme_t *t = ui_theme_create(template_def->name, THEME_MODE_CUSTOM);
    if (!t) return NULL;

    ui_theme_t *base = ui_theme_get(template_def->base_theme_name);
    if (base) {
        for (int i = 0; i < base->var_count; i++) {
            ui_theme_add_var(t, &base->variables[i]);
        }
    }

    for (int i = 0; i < template_def->override_count; i++) {
        char var_name[128];
        snprintf(var_name, sizeof(var_name), "--%s",
                 template_def->color_overrides[i].key);
        ui_theme_add_var(t, ui_theme_var_create(
            var_name,
            template_def->color_overrides[i].light,
            template_def->color_overrides[i].dark));
    }

    return t;
}

ui_theme_t* ui_theme_clone(const ui_theme_t *source, const char *new_name) {
    if (!source || !new_name) return NULL;
    ui_theme_t *t = ui_theme_create(new_name, source->mode);
    if (!t) return NULL;
    strncpy(t->description, source->description, 255);
    t->description[255] = '\0';
    t->color_scheme = source->color_scheme;
    for (int i = 0; i < source->var_count; i++) {
        ui_theme_add_var(t, &source->variables[i]);
    }
    return t;
}

void ui_theme_free(ui_theme_t *theme) {
    if (!theme) return;
    free(theme->variables);
    free(theme);
}

ui_theme_override_t* ui_theme_override_create(const char *selector,
                                              ui_theme_override_scope_t scope) {
    ui_theme_override_t *ov = (ui_theme_override_t*)calloc(1, sizeof(ui_theme_override_t));
    if (!ov) return NULL;
    ov->selector = selector;
    ov->scope = scope;
    ov->active = false;
    ov->override_capacity = 8;
    ov->overrides = (ui_theme_variable_t*)calloc(8, sizeof(ui_theme_variable_t));
    return ov;
}

void ui_theme_override_add_var(ui_theme_override_t *override,
                               const char *name,
                               const char *light_value,
                               const char *dark_value) {
    if (!override || !name) return;
    if (override->override_count >= override->override_capacity) {
        int nc = override->override_capacity * 2;
        ui_theme_variable_t *nv = (ui_theme_variable_t*)realloc(
            override->overrides, (size_t)nc * sizeof(ui_theme_variable_t));
        if (!nv) return;
        override->overrides = nv;
        override->override_capacity = nc;
    }
    override->overrides[override->override_count].name = name;
    override->overrides[override->override_count].light_value = light_value;
    override->overrides[override->override_count].dark_value = dark_value;
    override->override_count++;
}

void ui_theme_override_apply(ui_theme_override_t *override) {
    if (!override) return;
    override->active = true;
    if (!g_theme_manager) return;
    if (g_theme_manager->override_count < g_theme_manager->override_capacity) {
        g_theme_manager->active_overrides[g_theme_manager->override_count++] = *override;
    }
}

void ui_theme_override_remove(ui_theme_override_t *override) {
    if (!override || !g_theme_manager) return;
    for (int i = 0; i < g_theme_manager->override_count; i++) {
        if (&g_theme_manager->active_overrides[i] == override) {
            for (int j = i; j < g_theme_manager->override_count - 1; j++) {
                g_theme_manager->active_overrides[j] = g_theme_manager->active_overrides[j+1];
            }
            g_theme_manager->override_count--;
            break;
        }
    }
    override->active = false;
}

void ui_theme_override_clear_all(void) {
    if (!g_theme_manager) return;
    g_theme_manager->override_count = 0;
}

void ui_theme_override_free(ui_theme_override_t *override) {
    if (!override) return;
    free(override->overrides);
    free(override);
}

int ui_theme_to_css(const ui_theme_t *theme,
                    const ui_theme_css_options_t *options,
                    char *buffer, int buf_size) {
    if (!theme || !buffer || buf_size <= 0) return 0;
    ui_theme_css_options_t opts;
    if (options) opts = *options;
    else { memset(&opts, 0, sizeof(opts)); opts.include_root_selector = true; opts.indent_spaces = 2; }

    int pos = 0;
    if (opts.include_root_selector) {
        if (theme->mode == THEME_MODE_DARK && opts.nest_dark_media) {
            pos += snprintf(buffer + pos, (size_t)(buf_size - pos),
                            "@media (prefers-color-scheme: dark) {\n");
            pos += snprintf(buffer + pos, (size_t)(buf_size - pos), "  :root {\n");
        } else {
            pos += snprintf(buffer + pos, (size_t)(buf_size - pos), ":root {\n");
        }
    }

    char indent[16];
    int sp = opts.include_root_selector ? (opts.nest_dark_media ? 4 : 2) : 0;
    for (int i = 0; i < sp && i < 15; i++) indent[i] = ' '; indent[sp] = '\0';

    if (opts.include_color_scheme && theme->color_scheme != COLOR_SCHEME_NORMAL) {
        const char *cs = (theme->color_scheme == COLOR_SCHEME_DARK) ? "dark" : "light";
        pos += snprintf(buffer + pos, (size_t)(buf_size - pos), "%s--color-scheme: %s;\n", indent, cs);
    }

    for (int i = 0; i < theme->var_count; i++) {
        const char *name = theme->variables[i].name;
        const char *val  = theme->variables[i].light_value;
        if (!name || !val) continue;
        pos += snprintf(buffer + pos, (size_t)(buf_size - pos),
                        "%s%s: %s;\n", indent, name, val);
    }

    if (opts.include_root_selector) {
        if (theme->mode == THEME_MODE_DARK && opts.nest_dark_media) {
            pos += snprintf(buffer + pos, (size_t)(buf_size - pos), "  }\n}\n");
        } else {
            pos += snprintf(buffer + pos, (size_t)(buf_size - pos), "}\n");
        }
    }
    return pos;
}

int ui_theme_transition_to_css(const ui_theme_transition_t *transition,
                               char *buffer, int buf_size) {
    if (!transition || !buffer || buf_size <= 0) return 0;
    return snprintf(buffer, (size_t)buf_size,
        "*, *::before, *::after {\n"
        "  transition: background-color %dms %s, color %dms %s, "
        "border-color %dms %s, box-shadow %dms %s;\n"
        "}\n",
        transition->duration_ms, transition->easing ? transition->easing : "ease",
        transition->duration_ms, transition->easing ? transition->easing : "ease",
        transition->duration_ms, transition->easing ? transition->easing : "ease",
        transition->duration_ms, transition->easing ? transition->easing : "ease");
}

int ui_theme_generate_full_css(const ui_theme_context_t *ctx,
                               const ui_theme_css_options_t *options,
                               char *buffer, int buf_size) {
    if (!buffer || buf_size <= 0) return 0;
    int pos = 0;

    ui_theme_css_options_t light_opts;
    if (options) light_opts = *options;
    else { memset(&light_opts, 0, sizeof(light_opts)); light_opts.include_root_selector = true; light_opts.indent_spaces = 2; }
    light_opts.include_color_scheme = true;

    pos += ui_theme_to_css(g_default_light, &light_opts, buffer + pos, buf_size - pos);

    ui_theme_css_options_t dark_opts = light_opts;
    dark_opts.nest_dark_media = true;
    g_default_dark->mode = THEME_MODE_DARK;
    pos += ui_theme_to_css(g_default_dark, &dark_opts, buffer + pos, buf_size - pos);

    if (options && options->include_transition) {
        ui_theme_transition_t tr;
        memset(&tr, 0, sizeof(tr));
        tr.type = THEME_TRANSITION_FADE;
        tr.duration_ms = ctx ? ctx->transition_duration_ms : 300;
        tr.easing = ctx ? ctx->transition_easing : "ease-in-out";
        pos += ui_theme_transition_to_css(&tr, buffer + pos, buf_size - pos);
    }

    return pos;
}

int ui_theme_generate_dark_css(const ui_theme_t *theme,
                               const ui_theme_css_options_t *options,
                               char *buffer, int buf_size) {
    ui_theme_css_options_t dark_opts;
    if (options) dark_opts = *options;
    else { memset(&dark_opts, 0, sizeof(dark_opts)); dark_opts.include_root_selector = true; }
    dark_opts.nest_dark_media = true;
    ui_theme_t *dark_theme = (ui_theme_t*)theme;
    dark_theme->mode = THEME_MODE_DARK;
    return ui_theme_to_css(dark_theme, &dark_opts, buffer, buf_size);
}

bool ui_theme_save_mode(ui_theme_mode_t mode) { (void)mode; return true; }
ui_theme_mode_t ui_theme_load_mode(void) { return THEME_MODE_AUTO; }
bool ui_theme_save_custom(const ui_theme_t *theme) { (void)theme; return false; }
ui_theme_t* ui_theme_load_custom(const char *name) { (void)name; return NULL; }
void ui_theme_storage_clear(void) {}

void ui_theme_schedule_configure(const ui_theme_schedule_t *schedule) {
    if (!schedule || !g_theme_manager) return;
    g_theme_manager->schedule = *schedule;
}

bool ui_theme_schedule_should_switch(const ui_theme_schedule_t *schedule) {
    (void)schedule;
    return false;
}

const char* ui_theme_meta_color_scheme(ui_color_scheme_t scheme) {
    static const char *map[] = {
        "normal", "light", "dark", "light dark",
        "only light", "only dark"
    };
    if (scheme <= COLOR_SCHEME_ONLY_DARK) {
        static char buf[128];
        snprintf(buf, sizeof(buf),
            "<meta name=\"color-scheme\" content=\"%s\">", map[scheme]);
        return buf;
    }
    return "<meta name=\"color-scheme\" content=\"light dark\">";
}

const char* ui_theme_meta_theme_color(const char *hex_color) {
    static char buf[128];
    snprintf(buf, sizeof(buf),
        "<meta name=\"theme-color\" content=\"%s\">",
        hex_color ? hex_color : "#ffffff");
    return buf;
}

const char* ui_theme_meta_theme_color_dark(const char *hex_color) {
    static char buf[256];
    snprintf(buf, sizeof(buf),
        "<meta name=\"theme-color\" content=\"%s\" media=\"(prefers-color-scheme: dark)\">",
        hex_color ? hex_color : "#0f172a");
    return buf;
}

ui_theme_transition_t ui_theme_transition_create(ui_theme_transition_type_t type, int duration_ms) {
    ui_theme_transition_t tr;
    memset(&tr, 0, sizeof(tr));
    tr.type = type;
    tr.duration_ms = duration_ms;
    tr.easing = "ease-in-out";
    return tr;
}

void ui_theme_transition_start(const ui_theme_transition_t *transition) {
    (void)transition;
}

const char* ui_theme_mode_class(ui_theme_mode_t mode) {
    static const char *map[] = {"light", "dark", "auto", "custom"};
    if (mode <= THEME_MODE_CUSTOM) return map[mode];
    return "auto";
}

const char* ui_theme_mode_data_attr(ui_theme_mode_t mode) {
    static const char *map[] = {"light", "dark", "auto", "custom"};
    if (mode <= THEME_MODE_CUSTOM) return map[mode];
    return "auto";
}

bool ui_theme_var_name_valid(const char *name) {
    if (!name) return false;
    return name[0] == '-' && name[1] == '-';
}

ui_theme_t* ui_theme_merge(const ui_theme_t *base, const ui_theme_t *overrides,
                           const char *result_name) {
    if (!base || !overrides || !result_name) return NULL;
    ui_theme_t *merged = ui_theme_create(result_name, base->mode);
    if (!merged) return NULL;
    for (int i = 0; i < base->var_count; i++) {
        ui_theme_add_var(merged, &base->variables[i]);
    }
    for (int i = 0; i < overrides->var_count; i++) {
        ui_theme_remove_var(merged, overrides->variables[i].name);
        ui_theme_add_var(merged, &overrides->variables[i]);
    }
    return merged;
}
