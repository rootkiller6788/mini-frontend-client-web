#include "component_library.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static ui_component_catalog_t *g_default_catalog = NULL;
static ui_component_t         *g_component_registry[COMPONENT_COUNT] = {NULL};

static const char *component_names[] = {
    "Button", "Input", "Card", "Modal", "Table",
    "Checkbox", "Radio", "Select", "Textarea", "Toggle",
    "Tooltip", "Badge", "Avatar", "Dropdown", "Tabs",
    "Accordion", "Breadcrumb", "Pagination", "Progress", "Skeleton",
    "Toast", "Dialog", "Drawer"
};

static const char *component_tags[] = {
    "button", "input", "div", "div", "table",
    "input", "input", "select", "textarea", "button",
    "div", "span", "div", "div", "div",
    "div", "nav", "nav", "div", "div",
    "div", "div", "div"
};

static const char *component_categories[] = {
    "form", "form", "layout", "feedback", "data",
    "form", "form", "form", "form", "form",
    "feedback", "feedback", "display", "navigation", "navigation",
    "layout", "navigation", "navigation", "feedback", "feedback",
    "feedback", "feedback", "layout"
};

static const char *variant_names[] = {
    "primary", "secondary", "outline", "ghost",
    "destructive", "link", "success", "warning", "info"
};

static const char *size_names[] = {
    "xs", "sm", "md", "lg", "xl", "2xl", "full"
};

static const char *state_names[] = {
    "default", "hover", "active", "focus", "disabled", "loading",
    "error", "success", "selected", "checked", "expanded"
};

static const char *variant_css_templates[] = {
    /* PRIMARY */     "bg:var(--btn-bg-primary); color:var(--btn-text-primary);",
    /* SECONDARY */   "bg:var(--color-bg-secondary); color:var(--color-text-primary);",
    /* OUTLINE */     "bg:transparent; border:1px solid var(--color-border);",
    /* GHOST */       "bg:transparent; color:var(--color-text-primary);",
    /* DESTRUCTIVE */ "bg:var(--color-red-500); color:#fff;",
    /* LINK */        "bg:transparent; color:var(--color-blue-500); text-decoration:underline;",
    /* SUCCESS */     "bg:var(--color-green-500); color:#fff;",
    /* WARNING */     "bg:var(--color-yellow-500); color:#000;",
    /* INFO */        "bg:var(--color-blue-500); color:#fff;",
};

static const char *size_padding_templates[] = {
    /* XS */   "padding: 0.25rem 0.5rem; font-size: 0.75rem;",
    /* SM */   "padding: 0.375rem 0.75rem; font-size: 0.875rem;",
    /* MD */   "padding: 0.5rem 1rem; font-size: 1rem;",
    /* LG */   "padding: 0.75rem 1.5rem; font-size: 1.125rem;",
    /* XL */   "padding: 1rem 2rem; font-size: 1.25rem;",
    /* 2XL */  "padding: 1.25rem 2.5rem; font-size: 1.5rem;",
    /* FULL */ "padding: 0.75rem 1rem; width: 100%;",
};

static ui_component_t* ui_component_allocate(ui_component_type_t type) {
    ui_component_t *c = (ui_component_t*)calloc(1, sizeof(ui_component_t));
    if (!c) return NULL;
    c->type    = type;
    c->name    = (type < COMPONENT_COUNT) ? component_names[type] : "Unknown";
    c->tag     = (type < COMPONENT_COUNT) ? component_tags[type] : "div";
    c->category = (type < COMPONENT_COUNT) ? component_categories[type] : "generic";
    c->props   = ui_props_default(type);
    c->is_interactive = true;
    c->is_container   = false;
    return c;
}

void ui_component_lib_init(void) {
    g_default_catalog = ui_catalog_create("MiniUI Component Library", "1.0.0");

    for (int i = 0; i < COMPONENT_COUNT; i++) {
        g_component_registry[i] = NULL;
    }

    /* Button */
    ui_component_t *btn = ui_component_allocate(COMPONENT_BUTTON);
    btn->description = "A clickable button element for user interactions.";
    btn->css_base = "display:inline-flex; align-items:center; justify-content:center; "
                    "border-radius:var(--radius-md); cursor:pointer; transition:all 150ms; "
                    "border:none; font-weight:500; text-decoration:none;";
    btn->aria_roles = NULL;
    btn->aria_role_count = 0;
    g_component_registry[COMPONENT_BUTTON] = btn;
    ui_catalog_add_component(g_default_catalog, btn);

    /* Input */
    ui_component_t *inp = ui_component_allocate(COMPONENT_INPUT);
    inp->description = "A text input field for user data entry.";
    inp->css_base = "display:block; width:100%; border:1px solid var(--color-border); "
                    "border-radius:var(--radius-md); background:var(--color-bg-primary); "
                    "color:var(--color-text-primary); outline:none;";
    inp->is_interactive = true;
    g_component_registry[COMPONENT_INPUT] = inp;
    ui_catalog_add_component(g_default_catalog, inp);

    /* Card */
    ui_component_t *card = ui_component_allocate(COMPONENT_CARD);
    card->description = "A container card component for grouping content.";
    card->css_base = "background:var(--color-bg-primary); border:1px solid var(--color-border); "
                     "border-radius:var(--radius-lg); box-shadow:var(--shadow-sm); overflow:hidden;";
    card->is_interactive = false;
    card->is_container = true;
    g_component_registry[COMPONENT_CARD] = card;
    ui_catalog_add_component(g_default_catalog, card);

    /* Modal */
    ui_component_t *modal = ui_component_allocate(COMPONENT_MODAL);
    modal->description = "A modal dialog overlay.";
    modal->css_base = "position:fixed; inset:0; z-index:50; display:flex; align-items:center; "
                      "justify-content:center; background:rgba(0,0,0,0.5);";
    modal->is_container = true;
    g_component_registry[COMPONENT_MODAL] = modal;
    ui_catalog_add_component(g_default_catalog, modal);

    /* Table */
    ui_component_t *table = ui_component_allocate(COMPONENT_TABLE);
    table->description = "A data table component.";
    table->css_base = "width:100%; border-collapse:collapse;";
    table->is_interactive = false;
    table->is_container = true;
    g_component_registry[COMPONENT_TABLE] = table;
    ui_catalog_add_component(g_default_catalog, table);

    /* Remaining components - allocate basic definitions */
    for (int i = 5; i < COMPONENT_COUNT; i++) {
        ui_component_t *c = ui_component_allocate((ui_component_type_t)i);
        switch (i) {
            case COMPONENT_CHECKBOX:  c->css_base = "appearance:none; width:1rem; height:1rem; border:1px solid var(--color-border); border-radius:var(--radius-sm);"; break;
            case COMPONENT_TOOLTIP:   c->css_base = "position:relative;"; break;
            case COMPONENT_BADGE:     c->css_base = "display:inline-flex; align-items:center; border-radius:var(--radius-full);"; break;
            case COMPONENT_AVATAR:    c->css_base = "display:inline-flex; align-items:center; justify-content:center; border-radius:var(--radius-full); overflow:hidden;"; break;
            case COMPONENT_TABS:      c->css_base = "display:flex; border-bottom:1px solid var(--color-border);"; c->is_container = true; break;
            case COMPONENT_PROGRESS:  c->css_base = "width:100%; height:0.5rem; background:var(--color-bg-secondary); border-radius:var(--radius-full); overflow:hidden;"; break;
            case COMPONENT_TOAST:     c->css_base = "position:fixed; bottom:1rem; right:1rem; z-index:100;"; break;
            default: c->css_base = "display:block;"; break;
        }
        g_component_registry[i] = c;
        ui_catalog_add_component(g_default_catalog, c);
    }
}

void ui_component_lib_shutdown(void) {
    for (int i = 0; i < COMPONENT_COUNT; i++) {
        if (g_component_registry[i]) {
            ui_component_free(g_component_registry[i]);
            g_component_registry[i] = NULL;
        }
    }
    if (g_default_catalog) {
        ui_catalog_free(g_default_catalog);
        g_default_catalog = NULL;
    }
}

ui_component_t* ui_component_create(ui_component_type_t type,
                                    const char *name, const char *category) {
    ui_component_t *c = ui_component_allocate(type);
    if (c && name) c->name = name;
    if (c && category) c->category = category;
    return c;
}

ui_component_t* ui_component_get(ui_component_type_t type) {
    if (type >= COMPONENT_COUNT) return NULL;
    return g_component_registry[type];
}

ui_component_t* ui_component_get_by_name(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < COMPONENT_COUNT; i++) {
        if (g_component_registry[i] &&
            strcmp(g_component_registry[i]->name, name) == 0)
            return g_component_registry[i];
    }
    return NULL;
}

int ui_component_render_html(const ui_component_t *comp,
                             const ui_component_props_t *props,
                             const ui_render_context_t *ctx,
                             char *buffer, int buffer_size) {
    if (!comp || !buffer || buffer_size <= 0) return 0;
    const ui_component_props_t *p = props ? props : &comp->props;
    int indent = ctx ? ctx->indent_level : 0;
    int pos = 0;
    bool disabled = p->disabled || p->loading;

    /* Build class string */
    char class_str[256];
    snprintf(class_str, sizeof(class_str), "%s-%s-%s%s%s%s",
             comp->name, variant_names[p->variant], size_names[p->size],
             disabled ? " disabled" : "",
             p->loading ? " loading" : "",
             p->class_name ? " " : "", p->class_name ? p->class_name : "");

    /* Indentation */
    for (int i = 0; i < indent; i++) buffer[pos++] = ' ';
    buffer[pos] = '\0';

    if (comp->type == COMPONENT_BUTTON) {
        pos += snprintf(buffer + pos, (size_t)(buffer_size - pos),
            "<%s class=\"%s\"%s%s%s%s>%s</%s>",
            comp->tag, class_str,
            disabled ? " disabled" : "",
            p->id ? " id=\"" : "", p->id ? p->id : "", p->id ? "\"" : "",
            p->label ? p->label : (comp->name),
            comp->tag);
    } else if (comp->type == COMPONENT_INPUT) {
        pos += snprintf(buffer + pos, (size_t)(buffer_size - pos),
            "<%s class=\"%s\" type=\"text\" placeholder=\"%s\"%s%s%s%s />",
            comp->tag, class_str,
            p->placeholder ? p->placeholder : "",
            disabled ? " disabled" : "",
            p->id ? " id=\"" : "", p->id ? p->id : "", p->id ? "\"" : "",
            p->auto_focus ? " autofocus" : "");
    } else {
        pos += snprintf(buffer + pos, (size_t)(buffer_size - pos),
            "<%s class=\"%s\"%s%s%s>",
            comp->tag, class_str,
            disabled ? " disabled" : "",
            p->id ? " id=\"" : "", p->id ? p->id : "", p->id ? "\"" : "");
        if (p->label) pos += snprintf(buffer + pos, (size_t)(buffer_size - pos), "%s", p->label);
        pos += snprintf(buffer + pos, (size_t)(buffer_size - pos), "</%s>", comp->tag);
    }
    return pos;
}

int ui_component_render_css(const ui_component_t *comp,
                            const ui_component_props_t *props,
                            const ui_render_context_t *ctx,
                            char *buffer, int buffer_size) {
    if (!comp || !buffer || buffer_size <= 0) return 0;
    const ui_component_props_t *p = props ? props : &comp->props;
    int pos = 0;

    pos += snprintf(buffer + pos, (size_t)(buffer_size - pos),
        ".%s-%s-%s {\n",
        comp->name, variant_names[p->variant], size_names[p->size]);

    if (comp->css_base)
        pos += snprintf(buffer + pos, (size_t)(buffer_size - pos), "  %s\n", comp->css_base);
    if (variant_names[p->variant] && p->variant <= VARIANT_INFO)
        pos += snprintf(buffer + pos, (size_t)(buffer_size - pos), "  /* variant: %s */\n", variant_names[p->variant]);
    if (size_names[p->size] && p->size <= SIZE_FULL)
        pos += snprintf(buffer + pos, (size_t)(buffer_size - pos), "  %s\n", size_padding_templates[p->size]);

    if (p->full_width)
        pos += snprintf(buffer + pos, (size_t)(buffer_size - pos), "  width: 100%%;\n");

    if (p->disabled || comp->states.current == STATE_DISABLED) {
        pos += snprintf(buffer + pos, (size_t)(buffer_size - pos),
            "  opacity: 0.5; cursor: not-allowed; pointer-events: none;\n");
    }

    pos += snprintf(buffer + pos, (size_t)(buffer_size - pos), "}\n");
    return pos;
}

const char* ui_component_class(const ui_component_t *comp, ui_component_state_t state) {
    static char buf[128];
    if (!comp) return "";
    snprintf(buf, sizeof(buf), "%s--%s",
             comp->name, state <= STATE_EXPANDED ? state_names[state] : "default");
    return buf;
}

ui_component_catalog_t* ui_catalog_create(const char *name, const char *version) {
    ui_component_catalog_t *cat = (ui_component_catalog_t*)calloc(1, sizeof(ui_component_catalog_t));
    if (!cat) return NULL;
    cat->name = name;
    cat->version = version;
    cat->component_capacity = 32;
    cat->story_capacity = 64;
    cat->components = (ui_component_t**)calloc((size_t)cat->component_capacity, sizeof(ui_component_t*));
    cat->stories    = (ui_component_story_t**)calloc((size_t)cat->story_capacity, sizeof(ui_component_story_t*));
    return cat;
}

void ui_catalog_add_component(ui_component_catalog_t *catalog, ui_component_t *component) {
    if (!catalog || !component) return;
    if (catalog->component_count >= catalog->component_capacity) {
        int nc = catalog->component_capacity * 2;
        ui_component_t **na = (ui_component_t**)realloc(
            catalog->components, (size_t)nc * sizeof(ui_component_t*));
        if (!na) return;
        catalog->components = na;
        catalog->component_capacity = nc;
    }
    catalog->components[catalog->component_count++] = component;
}

void ui_catalog_add_story(ui_component_catalog_t *catalog, ui_component_story_t *story) {
    if (!catalog || !story) return;
    if (catalog->story_count >= catalog->story_capacity) {
        int nc = catalog->story_capacity * 2;
        ui_component_story_t **ns = (ui_component_story_t**)realloc(
            catalog->stories, (size_t)nc * sizeof(ui_component_story_t*));
        if (!ns) return;
        catalog->stories = ns;
        catalog->story_capacity = nc;
    }
    catalog->stories[catalog->story_count++] = story;
}

ui_component_story_t* ui_story_create(const char *name, ui_component_type_t comp_type,
                                      ui_component_props_t props) {
    ui_component_story_t *s = (ui_component_story_t*)calloc(1, sizeof(ui_component_story_t));
    if (!s) return NULL;
    s->name = name;
    s->component_type = comp_type;
    s->props = props;
    s->child_capacity = 4;
    s->children = (ui_component_story_t**)calloc((size_t)s->child_capacity, sizeof(ui_component_story_t*));
    return s;
}

void ui_story_add_child(ui_component_story_t *parent, ui_component_story_t *child) {
    if (!parent || !child) return;
    if (parent->child_count >= parent->child_capacity) {
        int nc = parent->child_capacity * 2;
        ui_component_story_t **nc_arr = (ui_component_story_t**)realloc(
            parent->children, (size_t)nc * sizeof(ui_component_story_t*));
        if (!nc_arr) return;
        parent->children = nc_arr;
        parent->child_capacity = nc;
    }
    parent->children[parent->child_count++] = child;
}

int ui_catalog_render_storybook(const ui_component_catalog_t *catalog,
                                const ui_render_context_t *ctx,
                                char *buffer, int buffer_size) {
    if (!catalog || !buffer || buffer_size <= 0) return 0;
    int pos = 0;
    pos += snprintf(buffer + pos, (size_t)(buffer_size - pos),
        "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
        "<meta charset=\"UTF-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "<title>%s v%s - Storybook</title>\n"
        "<style>\n"
        ":root {\n"
        "  --color-bg-primary: #fff;\n"
        "  --color-bg-secondary: #f8fafc;\n"
        "  --color-text-primary: #0f172a;\n"
        "  --color-text-secondary: #64748b;\n"
        "  --color-border: #e2e8f0;\n"
        "  --color-blue-500: #3b82f6;\n"
        "  --color-blue-600: #2563eb;\n"
        "  --color-red-500: #ef4444;\n"
        "  --color-green-500: #22c55e;\n"
        "  --color-yellow-500: #eab308;\n"
        "  --radius-sm: 0.125rem;\n"
        "  --radius-md: 0.375rem;\n"
        "  --radius-lg: 0.5rem;\n"
        "  --radius-full: 9999px;\n"
        "  --shadow-sm: 0 1px 2px 0 rgb(0 0 0 / 0.05);\n"
        "  --btn-bg-primary: var(--color-blue-500);\n"
        "  --btn-text-primary: #fff;\n"
        "}\n"
        "body { font-family: system-ui, sans-serif; background: #f1f5f9; margin: 0; padding: 2rem; }\n"
        ".storybook-header { margin-bottom: 2rem; }\n"
        ".storybook-header h1 { margin: 0; }\n"
        ".component-section { background: #fff; border-radius: 0.5rem; padding: 1.5rem; margin-bottom: 1.5rem; box-shadow: var(--shadow-sm); }\n"
        ".component-section h2 { margin-top: 0; border-bottom: 1px solid var(--color-border); padding-bottom: 0.5rem; }\n"
        ".story-entry { margin: 0.5rem 0; padding: 0.5rem; border: 1px dashed var(--color-border); border-radius: var(--radius-md); }\n"
        ".story-label { font-size: 0.75rem; color: var(--color-text-secondary); margin-bottom: 0.25rem; }\n"
        "</style>\n</head>\n<body>\n",
        catalog->name ? catalog->name : "Component Library",
        catalog->version ? catalog->version : "1.0.0");

    pos += snprintf(buffer + pos, (size_t)(buffer_size - pos),
        "<div class=\"storybook-header\">\n"
        "  <h1>%s</h1>\n"
        "  <p>Version %s | %d components | %d stories</p>\n"
        "</div>\n",
        catalog->name ? catalog->name : "Component Library",
        catalog->version ? catalog->version : "1.0.0",
        catalog->component_count, catalog->story_count);

    for (int i = 0; i < catalog->component_count; i++) {
        ui_component_t *comp = catalog->components[i];
        if (!comp) continue;
        pos += snprintf(buffer + pos, (size_t)(buffer_size - pos),
            "<div class=\"component-section\">\n"
            "  <h2>%s <span style=\"font-weight:normal;font-size:0.875rem;color:var(--color-text-secondary)\">(%s)</span></h2>\n"
            "  <p>%s</p>\n",
            comp->name, comp->category ? comp->category : "generic",
            comp->description ? comp->description : "");

        int variants[] = {VARIANT_PRIMARY, VARIANT_SECONDARY, VARIANT_OUTLINE, VARIANT_GHOST};
        int sizes[]    = {SIZE_SM, SIZE_MD, SIZE_LG};
        for (int v = 0; v < 4; v++) {
            for (int s = 0; s < 3; s++) {
                ui_component_props_t demo_props = ui_props_default(comp->type);
                demo_props.variant = (ui_variant_t)variants[v];
                demo_props.size    = (ui_size_t)sizes[s];
                pos += snprintf(buffer + pos, (size_t)(buffer_size - pos),
                    "  <div class=\"story-entry\">\n"
                    "    <div class=\"story-label\">%s / %s</div>\n",
                    variant_names[variants[v]], size_names[sizes[s]]);
                char html_buf[512];
                ui_render_context_t rctx = {RENDER_TARGET_HTML, false, false, false, 4};
                ui_component_render_html(comp, &demo_props, &rctx, html_buf, sizeof(html_buf));
                pos += snprintf(buffer + pos, (size_t)(buffer_size - pos), "    %s\n", html_buf);
                pos += snprintf(buffer + pos, (size_t)(buffer_size - pos), "  </div>\n");
            }
        }
        pos += snprintf(buffer + pos, (size_t)(buffer_size - pos), "</div>\n");
    }

    pos += snprintf(buffer + pos, (size_t)(buffer_size - pos), "</body>\n</html>\n");
    return pos;
}

int ui_component_render_state_matrix(const ui_component_t *comp,
                                     const ui_render_context_t *ctx,
                                     char *buffer, int buffer_size) {
    if (!comp || !buffer || buffer_size <= 0) return 0;
    int pos = snprintf(buffer, (size_t)buffer_size, "");
    for (int st = 0; st <= STATE_EXPANDED; st++) {
        pos += snprintf(buffer + pos, (size_t)(buffer_size - pos),
            "/* %s: %s */\n.%s--%s {\n  /* state-specific styles */\n}\n",
            comp->name, state_names[st], comp->name, state_names[st]);
    }
    return pos;
}

int ui_component_list_names(const char ***names) {
    if (names) *names = component_names;
    return COMPONENT_COUNT;
}

int ui_catalog_list_categories(const char ***categories) {
    if (categories) *categories = component_categories;
    return COMPONENT_COUNT;
}

bool ui_component_is_container(ui_component_type_t type) {
    if (type >= COMPONENT_COUNT) return false;
    return g_component_registry[type] ? g_component_registry[type]->is_container : false;
}

const char** ui_component_aria_roles(ui_component_type_t type, int *count) {
    static const char *btn_roles[] = {"button"};
    static const char *input_roles[] = {"textbox"};
    static const char *modal_roles[] = {"dialog"};
    static const char *table_roles[] = {"table"};
    static const char *nav_roles[] = {"navigation"};
    if (count) *count = 0;
    if (type >= COMPONENT_COUNT) return NULL;
    switch (type) {
        case COMPONENT_BUTTON:    if (count) *count = 1; return btn_roles;
        case COMPONENT_INPUT:     if (count) *count = 1; return input_roles;
        case COMPONENT_MODAL:     if (count) *count = 1; return modal_roles;
        case COMPONENT_TABLE:     if (count) *count = 1; return table_roles;
        case COMPONENT_BREADCRUMB:
        case COMPONENT_PAGINATION: if (count) *count = 1; return nav_roles;
        default: return NULL;
    }
}

int ui_component_generate_docs(const ui_component_t *comp,
                               char *buffer, int buf_size) {
    if (!comp || !buffer || buf_size <= 0) return 0;
    return snprintf(buffer, (size_t)buf_size,
        "## %s\n\n"
        "**Category:** %s\n"
        "**HTML Tag:** `<%s>`\n"
        "**Interactive:** %s\n"
        "**Container:** %s\n\n"
        "%s\n\n"
        "### Base CSS\n```css\n%s\n```\n",
        comp->name, comp->category ? comp->category : "generic",
        comp->tag,
        comp->is_interactive ? "Yes" : "No",
        comp->is_container ? "Yes" : "No",
        comp->description ? comp->description : "_No description_",
        comp->css_base ? comp->css_base : "");
}

ui_component_props_t ui_props_default(ui_component_type_t type) {
    ui_component_props_t p;
    memset(&p, 0, sizeof(p));
    p.variant   = VARIANT_PRIMARY;
    p.size      = SIZE_MD;
    p.disabled  = false;
    p.loading   = false;
    p.full_width = false;
    p.label     = (type < COMPONENT_COUNT) ? component_names[type] : "";
    p.placeholder = "";
    p.tab_index = 0;
    p.auto_focus = false;
    return p;
}

ui_component_props_t ui_props_with_variant(ui_component_type_t type, ui_variant_t variant) {
    ui_component_props_t p = ui_props_default(type);
    p.variant = variant;
    return p;
}

ui_component_props_t ui_props_with_size(ui_component_type_t type, ui_size_t size) {
    ui_component_props_t p = ui_props_default(type);
    p.size = size;
    return p;
}

void ui_component_free(ui_component_t *comp) {
    if (!comp) return;
    if (comp->aria_roles) free(comp->aria_roles);
    free(comp);
}

void ui_catalog_free(ui_component_catalog_t *catalog) {
    if (!catalog) return;
    if (catalog->components) {
        for (int i = 0; i < catalog->component_count; i++) {
            ui_component_free(catalog->components[i]);
        }
        free(catalog->components);
    }
    if (catalog->stories) {
        for (int i = 0; i < catalog->story_count; i++) {
            ui_story_free(catalog->stories[i]);
        }
        free(catalog->stories);
    }
    free(catalog);
}

void ui_story_free(ui_component_story_t *story) {
    if (!story) return;
    if (story->children) {
        for (int i = 0; i < story->child_count; i++) {
            ui_story_free(story->children[i]);
        }
        free(story->children);
    }
    free(story);
}
