#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/accessibility.h"

int main(void) {
    ui_a11y_init();
    printf("=== Accessibility & WCAG 2.1 Compliance Demo ===\n\n");

    /* ARIA Roles */
    printf("--- ARIA Landmark Roles ---\n");
    ui_aria_role_t landmarks[] = {
        ARIA_ROLE_BANNER, ARIA_ROLE_MAIN, ARIA_ROLE_NAVIGATION,
        ARIA_ROLE_COMPLEMENTARY, ARIA_ROLE_CONTENTINFO, ARIA_ROLE_FORM,
        ARIA_ROLE_SEARCH, ARIA_ROLE_REGION
    };
    for (int i = 0; i < 8; i++) {
        printf("  %-20s => %s (%s)\n",
               ui_aria_role_name(landmarks[i]),
               ui_aria_is_landmark(landmarks[i]) ? "landmark" : "-",
               ui_aria_is_widget(landmarks[i]) ? "widget" : "-");
    }

    printf("\n--- ARIA Widget Roles ---\n");
    ui_aria_role_t widgets[] = {
        ARIA_ROLE_BUTTON, ARIA_ROLE_CHECKBOX, ARIA_ROLE_TEXTBOX,
        ARIA_ROLE_SLIDER, ARIA_ROLE_SWITCH, ARIA_ROLE_TAB
    };
    for (int i = 0; i < 6; i++) {
        printf("  %-15s => %s\n",
               ui_aria_role_name(widgets[i]),
               ui_aria_is_widget(widgets[i]) ? "widget" : "static");
    }

    /* ARIA Properties */
    printf("\n--- ARIA Properties ---\n");
    ui_aria_prop_key_t prop_keys[] = {
        ARIA_PROP_LABEL, ARIA_PROP_EXPANDED, ARIA_PROP_SELECTED,
        ARIA_PROP_CHECKED, ARIA_PROP_DISABLED, ARIA_PROP_PRESSED,
        ARIA_PROP_HIDDEN, ARIA_PROP_LIVE, ARIA_PROP_MODAL
    };
    for (int i = 0; i < 9; i++) {
        printf("  %s\n", ui_aria_prop_key_name(prop_keys[i]));
    }

    /* Build accessibility tree */
    printf("\n--- Building Accessibility Tree ---\n");
    ui_a11y_node_t *page    = ui_a11y_node_create("Home Page", ARIA_ROLE_MAIN);
    ui_a11y_node_t *banner  = ui_a11y_node_create("Site Banner", ARIA_ROLE_BANNER);
    ui_a11y_node_t *nav     = ui_a11y_node_create("Main Navigation", ARIA_ROLE_NAVIGATION);
    ui_a11y_node_t *heading = ui_a11y_node_create("Welcome", ARIA_ROLE_HEADING);
    ui_a11y_node_t *btn1    = ui_a11y_node_create("Sign Up", ARIA_ROLE_BUTTON);
    ui_a11y_node_t *btn2    = ui_a11y_node_create("Learn More", ARIA_ROLE_BUTTON);
    ui_a11y_node_t *form    = ui_a11y_node_create("Login Form", ARIA_ROLE_FORM);
    ui_a11y_node_t *input   = ui_a11y_node_create("Email Input", ARIA_ROLE_TEXTBOX);
    ui_a11y_node_t *footer  = ui_a11y_node_create("Footer", ARIA_ROLE_CONTENTINFO);

    ui_a11y_node_set_prop(btn1,    ARIA_PROP_LABEL, "Create Account");
    ui_a11y_node_set_prop_bool(btn1, ARIA_PROP_EXPANDED, false);
    ui_a11y_node_set_prop(input,   ARIA_PROP_LABEL, "Email Address");
    ui_a11y_node_set_prop_bool(input, ARIA_PROP_REQUIRED, true);
    ui_a11y_node_set_prop(nav,     ARIA_PROP_LABEL, "Primary");

    ui_a11y_node_add_child(page, banner);
    ui_a11y_node_add_child(banner, nav);
    ui_a11y_node_add_child(page, heading);
    ui_a11y_node_add_child(page, btn1);
    ui_a11y_node_add_child(page, btn2);
    ui_a11y_node_add_child(page, form);
    ui_a11y_node_add_child(form, input);
    ui_a11y_node_add_child(page, footer);

    printf("  Tree built: %s\n", page->name);
    printf("  Direct children of page: %d\n", page->child_count);

    /* Print tree */
    char tree_text[4096];
    ui_a11y_tree_to_text(page, tree_text, sizeof(tree_text));
    printf("\n--- Accessibility Tree (Text) ---\n%s", tree_text);

    /* Assign tab order */
    printf("--- Tab Order ---\n");
    int tab_counter = 1;
    ui_a11y_assign_tab_order(page, &tab_counter);
    printf("  Total focusable elements: %d\n", tab_counter - 1);

    /* Focus management */
    printf("\n--- Focus Management ---\n");
    ui_focus_manager_t *fm = ui_focus_manager_create();
    ui_focus_set(fm, btn1);
    printf("  Current focus: %s\n", fm->current_focus->name);

    ui_focus_move(fm, FOCUS_DIRECTION_NEXT);
    printf("  After Next: %s\n", fm->current_focus->name);

    ui_focus_move(fm, FOCUS_DIRECTION_NEXT);
    printf("  After Next: %s\n", fm->current_focus->name);

    ui_focus_move(fm, FOCUS_DIRECTION_PREV);
    printf("  After Prev: %s\n", fm->current_focus->name);

    /* Focus trap (modal) */
    ui_focus_trap_begin(fm, form);
    printf("  Focus trapped in: %s\n", fm->is_trapped ? "Yes" : "No");
    ui_focus_trap_end(fm);

    /* Keyboard navigation */
    printf("\n--- Keyboard Navigation ---\n");
    ui_key_event_t key_events[] = {
        {KEY_TAB, false, false, false, false},
        {KEY_TAB, false, false, true, false},   /* Shift+Tab */
        {KEY_ENTER, false, false, false, false},
        {KEY_ESCAPE, false, false, false, false},
        {KEY_SPACE, false, false, false, false},
    };
    for (int i = 0; i < 5; i++) {
        const char *action = ui_keyboard_handle(fm, &key_events[i]);
        printf("  Key event %d: %s\n", i, action ? action : "(no action)");
    }

    /* Shortcut matching */
    printf("\n--- Keyboard Shortcuts ---\n");
    ui_key_event_t ctrl_s = {KEY_NONE, true, false, false, false};
    printf("  Ctrl+S match: %s\n",
           ui_keyboard_match_shortcut(&ctrl_s, "Ctrl+S") ? "Yes" : "No");

    /* Contrast ratio checker */
    printf("\n=== Contrast Ratio Checker ===\n");
    struct { const char *fg; const char *bg; const char *label; } tests[] = {
        {"#000000", "#FFFFFF", "Black on white"},
        {"#3B82F6", "#FFFFFF", "Blue btn on white"},
        {"#94A3B8", "#FFFFFF", "Light gray on white"},
        {"#64748B", "#0F172A", "Gray text on dark bg"},
        {"#EF4444", "#FFFFFF", "Red on white"},
        {"#FFFFFF", "#3B82F6", "White on blue"},
    };
    for (int i = 0; i < 6; i++) {
        ui_contrast_result_t res = ui_contrast_check_hex(tests[i].fg, tests[i].bg);
        printf("  %-22s => ratio: %.2f:1 | AA-Normal: %s | AA-Large: %s | AAA-Normal: %s\n",
               tests[i].label, res.ratio,
               res.passes_aa_normal ? "PASS" : "FAIL",
               res.passes_aa_large ? "PASS" : "FAIL",
               res.passes_aaa_normal ? "PASS" : "FAIL");
    }

    /* WCAG 2.1 Report */
    printf("\n=== WCAG 2.1 Compliance Report ===\n");
    ui_wcag_report_t *report = ui_wcag_report_create(WCAG_LEVEL_AA, "https://example.com");
    ui_wcag_check_all(report, page);
    char report_buf[4096];
    ui_wcag_report_full(report, report_buf, sizeof(report_buf));
    printf("%s", report_buf);

    /* Screen reader labels */
    printf("\n--- Screen Reader Labels ---\n");
    ui_sr_label_t *sr1 = ui_sr_label_create("Skip to main content", 0);
    ui_sr_label_t *sr2 = ui_sr_label_create("Navigation menu, 5 items", 1);
    char sr_html[256];
    ui_sr_label_render_html(sr1, sr_html, sizeof(sr_html));
    printf("  SR Label: %s\n", sr_html);
    ui_sr_label_render_html(sr2, sr_html, sizeof(sr_html));
    printf("  SR Label: %s\n", sr_html);

    /* Generate HTML from tree */
    printf("\n--- HTML from A11y Tree ---\n");
    char a11y_html[8192];
    ui_a11y_tree_to_html(page, a11y_html, sizeof(a11y_html));
    printf("%s", a11y_html);

    /* A11y IDs */
    char id1[64], id2[64];
    ui_a11y_generate_id(id1, sizeof(id1), "nav");
    ui_a11y_generate_id(id2, sizeof(id2), "btn");
    printf("Generated IDs: %s, %s\n", id1, id2);

    /* Cleanup */
    ui_sr_label_free(sr1);
    ui_sr_label_free(sr2);
    ui_focus_manager_free(fm);
    ui_wcag_report_free(report);
    ui_a11y_node_free(page);
    ui_a11y_shutdown();

    printf("\n=== Accessibility Demo Complete ===\n");
    return 0;
}
