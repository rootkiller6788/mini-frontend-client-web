#ifndef MINI_UI_ACCESSIBILITY_H
#define MINI_UI_ACCESSIBILITY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ===== ARIA Roles ===== */
typedef enum {
    ARIA_ROLE_ALERT            = 0,
    ARIA_ROLE_ALERTDIALOG      = 1,
    ARIA_ROLE_APPLICATION      = 2,
    ARIA_ROLE_ARTICLE          = 3,
    ARIA_ROLE_BANNER           = 4,
    ARIA_ROLE_BUTTON           = 5,
    ARIA_ROLE_CELL             = 6,
    ARIA_ROLE_CHECKBOX         = 7,
    ARIA_ROLE_COLUMNHEADER     = 8,
    ARIA_ROLE_COMBOBOX         = 9,
    ARIA_ROLE_COMMAND          = 10,
    ARIA_ROLE_COMPLEMENTARY    = 11,
    ARIA_ROLE_COMPOSITE        = 12,
    ARIA_ROLE_CONTENTINFO      = 13,
    ARIA_ROLE_DEFINITION       = 14,
    ARIA_ROLE_DIALOG           = 15,
    ARIA_ROLE_DIRECTORY        = 16,
    ARIA_ROLE_DOCUMENT         = 17,
    ARIA_ROLE_FEED             = 18,
    ARIA_ROLE_FIGURE           = 19,
    ARIA_ROLE_FORM             = 20,
    ARIA_ROLE_GRID             = 21,
    ARIA_ROLE_GRIDCELL         = 22,
    ARIA_ROLE_GROUP            = 23,
    ARIA_ROLE_HEADING          = 24,
    ARIA_ROLE_IMG              = 25,
    ARIA_ROLE_INPUT            = 26,
    ARIA_ROLE_LINK             = 27,
    ARIA_ROLE_LIST             = 28,
    ARIA_ROLE_LISTBOX          = 29,
    ARIA_ROLE_LISTITEM         = 30,
    ARIA_ROLE_LOG              = 31,
    ARIA_ROLE_MAIN             = 32,
    ARIA_ROLE_MARQUEE          = 33,
    ARIA_ROLE_MATH             = 34,
    ARIA_ROLE_MENU             = 35,
    ARIA_ROLE_MENUBAR          = 36,
    ARIA_ROLE_MENUITEM         = 37,
    ARIA_ROLE_MENUITEMCHECKBOX = 38,
    ARIA_ROLE_MENUITEMRADIO    = 39,
    ARIA_ROLE_NAVIGATION       = 40,
    ARIA_ROLE_NONE             = 41,
    ARIA_ROLE_NOTE             = 42,
    ARIA_ROLE_OPTION           = 43,
    ARIA_ROLE_PRESENTATION     = 44,
    ARIA_ROLE_PROGRESSBAR      = 45,
    ARIA_ROLE_RADIO            = 46,
    ARIA_ROLE_RADIOGROUP       = 47,
    ARIA_ROLE_RANGE            = 48,
    ARIA_ROLE_REGION           = 49,
    ARIA_ROLE_ROW              = 50,
    ARIA_ROLE_ROWGROUP         = 51,
    ARIA_ROLE_ROWHEADER        = 52,
    ARIA_ROLE_SCROLLBAR        = 53,
    ARIA_ROLE_SEARCH           = 54,
    ARIA_ROLE_SEARCHBOX        = 55,
    ARIA_ROLE_SEPARATOR        = 56,
    ARIA_ROLE_SLIDER           = 57,
    ARIA_ROLE_SPINBUTTON       = 58,
    ARIA_ROLE_STATUS           = 59,
    ARIA_ROLE_STRUCTURE        = 60,
    ARIA_ROLE_SWITCH           = 61,
    ARIA_ROLE_TAB              = 62,
    ARIA_ROLE_TABLE            = 63,
    ARIA_ROLE_TABLIST          = 64,
    ARIA_ROLE_TABPANEL         = 65,
    ARIA_ROLE_TERM             = 66,
    ARIA_ROLE_TEXTBOX          = 67,
    ARIA_ROLE_TIMER            = 68,
    ARIA_ROLE_TOOLBAR          = 69,
    ARIA_ROLE_TOOLTIP          = 70,
    ARIA_ROLE_TREE             = 71,
    ARIA_ROLE_TREEGRID         = 72,
    ARIA_ROLE_TREEITEM         = 73,
    ARIA_ROLE_WIDGET           = 74,
    ARIA_ROLE_WINDOW           = 75,
    ARIA_ROLE_COUNT            = 76,
} ui_aria_role_t;

/* ===== ARIA Property Keys ===== */
typedef enum {
    ARIA_PROP_LABEL            = 0,
    ARIA_PROP_LABELLEDBY       = 1,
    ARIA_PROP_DESCRIBEDBY      = 2,
    ARIA_PROP_DETAILS          = 3,
    ARIA_PROP_EXPANDED         = 4,
    ARIA_PROP_HASPOPUP         = 5,
    ARIA_PROP_HIDDEN           = 6,
    ARIA_PROP_INVALID          = 7,
    ARIA_PROP_PRESSED          = 8,
    ARIA_PROP_SELECTED         = 9,
    ARIA_PROP_CHECKED          = 10,
    ARIA_PROP_DISABLED         = 11,
    ARIA_PROP_READONLY         = 12,
    ARIA_PROP_REQUIRED         = 13,
    ARIA_PROP_MULTILINE        = 14,
    ARIA_PROP_MULTISELECTABLE  = 15,
    ARIA_PROP_ORIENTATION      = 16,
    ARIA_PROP_CURRENT          = 17,
    ARIA_PROP_ATOMIC           = 18,
    ARIA_PROP_BUSY             = 19,
    ARIA_PROP_CONTROLS         = 20,
    ARIA_PROP_FLOWTO           = 21,
    ARIA_PROP_LEVEL            = 22,
    ARIA_PROP_LIVE             = 23,
    ARIA_PROP_MODAL            = 24,
    ARIA_PROP_POSINSET         = 25,
    ARIA_PROP_SETSIZE          = 26,
    ARIA_PROP_SORT             = 27,
    ARIA_PROP_VALUEMAX         = 28,
    ARIA_PROP_VALUEMIN         = 29,
    ARIA_PROP_VALUENOW         = 30,
    ARIA_PROP_VALUETEXT        = 31,
    ARIA_PROP_ROWCOUNT         = 32,
    ARIA_PROP_COLCOUNT         = 33,
    ARIA_PROP_ROWINDEX         = 34,
    ARIA_PROP_COLINDEX         = 35,
    ARIA_PROP_ROWSPAN          = 36,
    ARIA_PROP_COLSPAN          = 37,
    ARIA_PROP_AUTOCOMPLETE     = 38,
    ARIA_PROP_ERRORMESSAGE     = 39,
    ARIA_PROP_HASPOPUP_MENU    = 40,
    ARIA_PROP_COUNT            = 41,
} ui_aria_prop_key_t;

/* ===== ARIA Live Regions ===== */
typedef enum {
    ARIA_LIVE_OFF       = 0,
    ARIA_LIVE_POLITE    = 1,
    ARIA_LIVE_ASSERTIVE = 2,
} ui_aria_live_t;

/* ===== ARIA Property Value ===== */
typedef struct {
    ui_aria_prop_key_t key;
    const char        *value;          /* string value */
    bool               boolean_value;
    int                int_value;
    bool               has_bool;
    bool               has_int;
} ui_aria_prop_t;

/* ===== Accessibility Node (for accessibility tree) ===== */
typedef struct ui_a11y_node_s {
    const char         *name;           /* accessible name */
    const char         *description;    /* accessible description */
    ui_aria_role_t      role;
    int                 role_index;     /* position in tree */
    ui_aria_prop_t     *properties;
    int                 prop_count;
    int                 prop_capacity;
    int                 tab_order;      /* -1 = not focusable */
    bool                focused;
    bool                hidden;
    int                 level;          /* heading level, nesting level */
    struct ui_a11y_node_s **children;
    int                 child_count;
    int                 child_capacity;
    struct ui_a11y_node_s *parent;
    void               *element_ref;    /* back-reference to UI element */
} ui_a11y_node_t;

/* ===== Focus Manager ===== */
typedef enum {
    FOCUS_DIRECTION_NEXT      = 0,
    FOCUS_DIRECTION_PREV      = 1,
    FOCUS_DIRECTION_UP        = 2,
    FOCUS_DIRECTION_DOWN      = 3,
    FOCUS_DIRECTION_LEFT      = 4,
    FOCUS_DIRECTION_RIGHT     = 5,
    FOCUS_DIRECTION_FIRST     = 6,
    FOCUS_DIRECTION_LAST      = 7,
} ui_focus_direction_t;

typedef enum {
    FOCUS_TRAP_MODE_NONE      = 0,
    FOCUS_TRAP_MODE_MODAL     = 1,  /* trap focus within container */
    FOCUS_TRAP_MODE_ROVING    = 2,  /* roving tabindex pattern */
} ui_focus_trap_mode_t;

typedef struct ui_focus_manager_s {
    ui_a11y_node_t    *current_focus;
    ui_a11y_node_t    *focus_scope;     /* root of focusable subtree */
    ui_focus_trap_mode_t trap_mode;
    bool               is_trapped;
    int                tab_sequence_counter;
} ui_focus_manager_t;

/* ===== Keyboard Navigation ===== */
typedef enum {
    KEY_NONE        = 0,
    KEY_TAB         = 9,
    KEY_ENTER       = 13,
    KEY_ESCAPE      = 27,
    KEY_SPACE       = 32,
    KEY_LEFT        = 37,
    KEY_UP          = 38,
    KEY_RIGHT       = 39,
    KEY_DOWN        = 40,
    KEY_HOME        = 36,
    KEY_END         = 35,
    KEY_PAGEUP      = 33,
    KEY_PAGEDOWN    = 34,
    KEY_SHIFT_TAB   = 256,  /* synthetic */
} ui_key_code_t;

typedef struct {
    ui_key_code_t key;
    bool          ctrl;
    bool          alt;
    bool          shift;
    bool          meta;
} ui_key_event_t;

/* ===== Contrast Ratio ===== */
typedef struct {
    uint8_t  fg_r, fg_g, fg_b;      /* foreground color */
    uint8_t  bg_r, bg_g, bg_b;      /* background color */
    float    ratio;                   /* calculated contrast ratio */
    bool     passes_aa_large;         /* WCAG AA large text (3:1) */
    bool     passes_aa_normal;        /* WCAG AA normal text (4.5:1) */
    bool     passes_aaa_large;        /* WCAG AAA large text (4.5:1) */
    bool     passes_aaa_normal;       /* WCAG AAA normal text (7:1) */
} ui_contrast_result_t;

/* ===== WCAG Compliance ===== */
typedef enum {
    WCAG_LEVEL_A  = 0,
    WCAG_LEVEL_AA = 1,
    WCAG_LEVEL_AAA = 2,
} ui_wcag_level_t;

typedef enum {
    WCAG_CRITERION_PERCEIVABLE = 0,  /* Principle 1 */
    WCAG_CRITERION_OPERABLE    = 1,  /* Principle 2 */
    WCAG_CRITERION_UNDERSTANDABLE = 2, /* Principle 3 */
    WCAG_CRITERION_ROBUST      = 3,  /* Principle 4 */
} ui_wcag_principle_t;

typedef struct {
    const char         *id;           /* e.g. "1.1.1" */
    const char         *name;         /* e.g. "Non-text Content" */
    ui_wcag_principle_t principle;
    ui_wcag_level_t     level;
    bool                passed;
    const char         *description;
    const char         *recommendation;
} ui_wcag_criterion_t;

/* ===== WCAG Audit Report ===== */
#define UI_WCAG_MAX_CRITERIA 78
typedef struct {
    ui_wcag_criterion_t criteria[UI_WCAG_MAX_CRITERIA];
    int                 criteria_count;
    int                 passed_count;
    int                 failed_count;
    int                 not_applicable_count;
    ui_wcag_level_t     target_level;
    const char         *url;
    const char         *timestamp;
    float               score;         /* 0.0 - 100.0 */
} ui_wcag_report_t;

/* ===== Screen Reader Labels ===== */
typedef struct {
    const char *text;
    const char *lang;          /* e.g. "en", "zh-CN" */
    int         priority;      /* 0 = critical, 1 = important, 2 = normal */
    bool        is_hidden;     /* visually hidden but available to SR */
} ui_sr_label_t;

/* ===== API ===== */

/* Initialize accessibility subsystem */
void ui_a11y_init(void);
void ui_a11y_shutdown(void);

/* --- ARIA --- */

/* Get ARIA role name as HTML attribute value */
const char* ui_aria_role_name(ui_aria_role_t role);

/* Get ARIA role description */
const char* ui_aria_role_description(ui_aria_role_t role);

/* Get ARIA property key as HTML attribute name */
const char* ui_aria_prop_key_name(ui_aria_prop_key_t key);

/* Check if a role requires specific properties */
int ui_aria_required_props(ui_aria_role_t role, ui_aria_prop_key_t **required, int *count);

/* Check if a role supports specific properties */
int ui_aria_supported_props(ui_aria_role_t role, ui_aria_prop_key_t **supported, int *count);

/* Check if a role is a landmark */
bool ui_aria_is_landmark(ui_aria_role_t role);

/* Check if a role is interactive/widget */
bool ui_aria_is_widget(ui_aria_role_t role);

/* Generate ARIA HTML string for a set of properties */
int ui_aria_render_props(
    const ui_aria_prop_t *props, int prop_count,
    char *buffer, int buf_size);

/* --- Accessibility Tree --- */

/* Create an accessibility tree node */
ui_a11y_node_t* ui_a11y_node_create(
    const char     *name,
    ui_aria_role_t  role);

/* Add a child to an accessibility node */
void ui_a11y_node_add_child(ui_a11y_node_t *parent, ui_a11y_node_t *child);

/* Set ARIA property on a node */
void ui_a11y_node_set_prop(
    ui_a11y_node_t    *node,
    ui_aria_prop_key_t key,
    const char        *value);

/* Set boolean ARIA property */
void ui_a11y_node_set_prop_bool(
    ui_a11y_node_t    *node,
    ui_aria_prop_key_t key,
    bool               value);

/* Set integer ARIA property */
void ui_a11y_node_set_prop_int(
    ui_a11y_node_t    *node,
    ui_aria_prop_key_t key,
    int                value);

/* Remove all properties from a node */
void ui_a11y_node_clear_props(ui_a11y_node_t *node);

/* Serialize accessibility tree to JSON for debugging */
int ui_a11y_tree_to_json(
    const ui_a11y_node_t *root,
    char *buffer, int buf_size);

/* Render accessibility tree as indented text */
int ui_a11y_tree_to_text(
    const ui_a11y_node_t *root,
    char *buffer, int buf_size);

/* Generate HTML with proper ARIA attributes from tree */
int ui_a11y_tree_to_html(
    const ui_a11y_node_t *root,
    char *buffer, int buf_size);

/* Free accessibility tree */
void ui_a11y_node_free(ui_a11y_node_t *node);

/* --- Focus Management --- */

/* Create focus manager */
ui_focus_manager_t* ui_focus_manager_create(void);

/* Set focus to a specific node */
void ui_focus_set(ui_focus_manager_t *fm, ui_a11y_node_t *node);

/* Move focus in a direction */
ui_a11y_node_t* ui_focus_move(
    ui_focus_manager_t  *fm,
    ui_focus_direction_t direction);

/* Get currently focused node */
ui_a11y_node_t* ui_focus_get_current(const ui_focus_manager_t *fm);

/* Trap focus within a scope (for modals) */
void ui_focus_trap_begin(ui_focus_manager_t *fm, ui_a11y_node_t *scope);
void ui_focus_trap_end(ui_focus_manager_t *fm);

/* Generate tab order for a tree */
void ui_a11y_assign_tab_order(ui_a11y_node_t *root, int *counter);

/* Free focus manager */
void ui_focus_manager_free(ui_focus_manager_t *fm);

/* --- Keyboard Navigation --- */

/* Handle key event and return action string (or NULL) */
const char* ui_keyboard_handle(
    ui_focus_manager_t *fm,
    ui_key_event_t     *event);

/* Check if key combination matches a shortcut pattern */
bool ui_keyboard_match_shortcut(
    const ui_key_event_t *event,
    const char           *shortcut_def); /* e.g. "Ctrl+S" */

/* --- Contrast Ratio Checker --- */

/* Calculate relative luminance (sRGB) */
float ui_color_luminance(uint8_t r, uint8_t g, uint8_t b);

/* Calculate contrast ratio between two colors */
float ui_contrast_ratio(
    uint8_t fg_r, uint8_t fg_g, uint8_t fg_b,
    uint8_t bg_r, uint8_t bg_g, uint8_t bg_b);

/* Check contrast against WCAG levels */
ui_contrast_result_t ui_contrast_check(
    uint8_t fg_r, uint8_t fg_g, uint8_t fg_b,
    uint8_t bg_r, uint8_t bg_g, uint8_t bg_b);

/* Convert hex color string to RGB */
bool ui_color_hex_to_rgb(const char *hex, uint8_t *r, uint8_t *g, uint8_t *b);

/* Check color hex pair for contrast */
ui_contrast_result_t ui_contrast_check_hex(
    const char *fg_hex, const char *bg_hex);

/* --- WCAG 2.1 Compliance Checker --- */

/* Initialize a WCAG report */
ui_wcag_report_t* ui_wcag_report_create(
    ui_wcag_level_t target_level,
    const char     *url);

/* Run all applicable WCAG criteria checks */
void ui_wcag_check_all(
    ui_wcag_report_t *report,
    const ui_a11y_node_t *root);

/* Check a single WCAG criterion */
bool ui_wcag_check_criterion(
    ui_wcag_report_t       *report,
    const char             *criterion_id,
    const ui_a11y_node_t   *root);

/* Print WCAG report summary */
int ui_wcag_report_summary(
    const ui_wcag_report_t *report,
    char *buffer, int buf_size);

/* Print WCAG report (full details) */
int ui_wcag_report_full(
    const ui_wcag_report_t *report,
    char *buffer, int buf_size);

/* Free WCAG report */
void ui_wcag_report_free(ui_wcag_report_t *report);

/* --- Screen Reader Labels --- */

/* Create screen reader label */
ui_sr_label_t* ui_sr_label_create(
    const char *text, int priority);

/* Render SR-only CSS class + HTML span */
int ui_sr_label_render_html(
    const ui_sr_label_t *label,
    char *buffer, int buf_size);

/* Free screen reader label */
void ui_sr_label_free(ui_sr_label_t *label);

/* --- Utility --- */

/* Get the recommended heading level for a node depth */
int ui_a11y_heading_level(int depth);

/* Validate ARIA role is allowed as child of parent role */
bool ui_aria_valid_child(
    ui_aria_role_t parent_role,
    ui_aria_role_t child_role);

/* Generate an accessible ID */
void ui_a11y_generate_id(char *buffer, int buf_size, const char *prefix);

#endif /* MINI_UI_ACCESSIBILITY_H */
