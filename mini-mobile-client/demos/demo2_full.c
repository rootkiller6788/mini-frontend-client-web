#include "react_native_bridge.h"
#include "native_modules.h"
#include "yoga_layout.h"
#include "list_virtualizer.h"
#include "push_notif.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static float cell_heights[200];
static const char *cell_labels[200];
static char cell_label_buf[200][32];

static void *demo_create(void)          { printf("  [lifecycle] create\n");  return (void *)1; }
static int  demo_start(void *ins)       { printf("  [lifecycle] start(%p)\n", ins); return 0; }
static int  demo_stop(void *ins)        { printf("  [lifecycle] stop(%p)\n", ins);  return 0; }
static void demo_destroy(void *ins)     { printf("  [lifecycle] destroy(%p)\n", ins); }

static int demo_set_config(const char *args, char *result, size_t len, void *ctx)
{
    (void)ctx;
    snprintf(result, len, "{\"config_set\":true,\"args\":%s}", args ? args : "null");
    return 0;
}

static int demo_get_data(const char *args, char *result, size_t len, void *ctx)
{
    (void)ctx; (void)args;
    snprintf(result, len, "{\"items\":[1,2,3,4,5],\"total\":5,\"timestamp\":%lld}",
             (long long)time(NULL));
    return 0;
}

static int demo_action(const char *args, char *result, size_t len, void *ctx)
{
    (void)ctx;
    snprintf(result, len, "{\"action\":\"completed\",\"input\":%s}", args ? args : "null");
    return 0;
}

static nm_method_impl g_demo_impls[] = { demo_set_config, demo_get_data, demo_action };

static yg_measure_result_t complex_measure(void *ctx, float w, yg_unit_t wm,
                                            float h, yg_unit_t hm)
{
    (void)wm; (void)hm;
    const char *txt = (const char *)ctx;
    size_t slen = txt ? strlen(txt) : 0;
    yg_measure_result_t r;
    r.width  = w > 10.0f ? (slen * 8.5f + 14.0f > w ? w : slen * 8.5f + 14.0f) : slen * 8.5f + 14.0f;
    r.height = h > 10.0f ? h : 28.0f;
    r.measured = true;
    return r;
}

static float lv_height_for_index(int index, lv_cell_type_t type, void *ctx)
{
    (void)type; (void)ctx;
    if (index >= 0 && index < 200) return cell_heights[index];
    return 60.0f;
}

static lv_cell_t *lv_do_render(int index, lv_cell_t *recycled, void *ctx)
{
    (void)recycled; (void)ctx;
    static lv_cell_t cell;
    memset(&cell, 0, sizeof(cell));
    cell.id      = index;
    cell.type    = LV_CELL_ITEM;
    cell.index   = index;
    cell.height  = cell_heights[index];
    cell.width   = 375.0f;
    cell.data    = (void *)cell_labels[index];
    cell.visible = true;
    return &cell;
}

static void lv_scroll_cb(float offset, lv_scroll_state_t st, void *ctx)
{
    (void)ctx;
    printf("  [list scroll] offset=%.0f state=%d\n", offset, (int)st);
}

static void lv_refresh_cb(void *ctx)
{
    (void)ctx;
    printf("  [list] REFRESH triggered\n");
}

static void lv_end_cb(void *ctx)
{
    (void)ctx;
    printf("  [list] END REACHED — loading more\n");
}

static void lv_press_cb(int index, void *ctx)
{
    (void)ctx;
    printf("  [list] TAP on cell %d (label: %s)\n", index,
           cell_labels[index] ? cell_labels[index] : "?");
}

static void token_callback(const char *token, void *ctx) {
    (void)ctx;
    printf("  [push] TOKEN: %.48s...\n", token);
}
static void recv_callback(const pn_notification_t *n, pn_app_state_t s, void *ctx) {
    (void)ctx;
    printf("  [push] RECV [%s]: %s (state=%d, badge=%d)\n",
           n->payload.title, n->payload.body, (int)s, n->payload.badge);
}
static void act_callback(const char *aid, const pn_notification_t *n, void *ctx) {
    (void)ctx;
    printf("  [push] ACTION '%s' on '%s'\n", aid, n->payload.title);
}
static void dl_callback(const char *link, void *ctx) {
    (void)ctx;
    printf("  [push] DEEPLINK => %s\n", link);
}

static void demo_section_1(void)
{
    printf("\n===== SECTION 1 — BRIDGE + MODULES INTEGRATION ====\n\n");

    rn_bridge_t bridge;
    rn_bridge_init(&bridge);

    nm_registry_t reg;
    nm_registry_init(&reg);
    nm_set_permission(&reg, NM_PERM_STORAGE, NM_PERM_GRANTED);

    nm_module_def_t def = {0};
    strcpy(def.name, "DemoModule");
    def.description   = "Integrated demo module";
    def.create        = demo_create;
    def.start         = demo_start;
    def.stop          = demo_stop;
    def.destroy       = demo_destroy;

    def.constants[0] = (nm_constant_t){ .name = "VERSION", .type = NM_CONST_INT, .value.int_val = 100 };
    def.constants[1] = (nm_constant_t){ .name = "DEBUG", .type = NM_CONST_BOOL, .value.bool_val = true };
    def.constant_count = 2;

    def.methods[0] = (nm_method_def_t){ .name = "setConfig", .is_async = false, .param_count = 1 };
    def.methods[1] = (nm_method_def_t){ .name = "getData", .is_async = true, .param_count = 0 };
    def.methods[2] = (nm_method_def_t){ .name = "action", .is_async = false, .param_count = 2 };
    def.method_count  = 3;
    def.method_impls  = g_demo_impls;
    def.perm_count    = 0;

    nm_register_module(&reg, &def);
    nm_start_module(&reg, "DemoModule");

    char result[256];
    nm_call_method(&reg, "DemoModule", "setConfig", "{\"theme\":\"dark\"}", result, sizeof(result));
    printf("  setConfig => %s\n", result);

    nm_call_method(&reg, "DemoModule", "getData", "{}", result, sizeof(result));
    printf("  getData   => %s\n", result);

    nm_call_method(&reg, "DemoModule", "action", "{\"id\":1}", result, sizeof(result));
    printf("  action    => %s\n", result);

    char *enc = rn_bridge_encode_call("DemoModule", "getData", "{\"force\":true}");
    printf("  Bridge encode: %s\n", enc);

    char mod[64], met[64], arg[256];
    rn_bridge_decode_call(enc, mod, met, arg, sizeof(arg));
    printf("  Bridge decode: module=%s method=%s\n", mod, met);

    rn_bridge_send_message(&bridge, RN_THREAD_UI, mod, met, arg);
    printf("  Bridge pending before flush: %d\n", rn_bridge_pending_count(&bridge));
    rn_bridge_flush(&bridge, RN_THREAD_UI);
    printf("  Bridge pending after flush: %d\n", rn_bridge_pending_count(&bridge));

    free(enc);
    nm_stop_module(&reg, "DemoModule");
    nm_registry_destroy(&reg);
    rn_bridge_destroy(&bridge);
}

static void demo_section_2(void)
{
    printf("\n===== SECTION 2 — COMPLEX LAYOUT ====\n\n");

    yg_node_t *page = yg_node_create();
    yg_node_set_width(page, 414.0f, YG_UNIT_POINT);
    yg_node_set_height(page, 896.0f, YG_UNIT_POINT);
    yg_node_set_flex_direction(page, YG_FLEX_DIRECTION_COLUMN);

    yg_node_t *navbar = yg_node_create();
    yg_node_set_height(navbar, 88.0f, YG_UNIT_POINT);
    yg_node_set_padding(navbar, YG_EDGE_TOP, 44.0f, YG_UNIT_POINT);
    yg_node_set_padding(navbar, YG_EDGE_LEFT, 16.0f, YG_UNIT_POINT);
    yg_node_set_padding(navbar, YG_EDGE_RIGHT, 16.0f, YG_UNIT_POINT);
    yg_node_set_justify_content(navbar, YG_JUSTIFY_CENTER);
    yg_node_set_measure_func(navbar, complex_measure, (void *)"Navigation Bar");
    yg_node_add_child(page, navbar);

    yg_node_t *scroll_area = yg_node_create();
    yg_node_set_flex_grow(scroll_area, 1.0f);
    yg_node_set_flex_direction(scroll_area, YG_FLEX_DIRECTION_COLUMN);
    yg_node_set_padding(scroll_area, YG_EDGE_LEFT, 16.0f, YG_UNIT_POINT);
    yg_node_set_padding(scroll_area, YG_EDGE_RIGHT, 16.0f, YG_UNIT_POINT);
    yg_node_add_child(page, scroll_area);

    yg_node_t *hero = yg_node_create();
    yg_node_set_height(hero, 200.0f, YG_UNIT_POINT);
    yg_node_set_margin(hero, YG_EDGE_TOP, 16.0f, YG_UNIT_POINT);
    yg_node_set_margin(hero, YG_EDGE_BOTTOM, 16.0f, YG_UNIT_POINT);
    yg_node_set_justify_content(hero, YG_JUSTIFY_CENTER);
    yg_node_set_align_items(hero, YG_ALIGN_CENTER);
    yg_node_set_measure_func(hero, complex_measure, (void *)"Hero Banner");
    yg_node_add_child(scroll_area, hero);

    yg_node_t *row_horz = yg_node_create();
    yg_node_set_flex_direction(row_horz, YG_FLEX_DIRECTION_ROW);
    yg_node_set_justify_content(row_horz, YG_JUSTIFY_SPACE_BETWEEN);
    yg_node_set_height(row_horz, 80.0f, YG_UNIT_POINT);
    yg_node_set_margin(row_horz, YG_EDGE_BOTTOM, 12.0f, YG_UNIT_POINT);
    yg_node_add_child(scroll_area, row_horz);

    const char *stats[] = {"Orders: 42", "Revenue: $1.2k", "Users: 1.5k"};
    for (int i = 0; i < 3; i++) {
        yg_node_t *stat = yg_node_create();
        yg_node_set_flex_grow(stat, 1.0f);
        yg_node_set_height(stat, 72.0f, YG_UNIT_POINT);
        yg_node_set_margin(stat, YG_EDGE_RIGHT, (i < 2) ? 8.0f : 0.0f, YG_UNIT_POINT);
        yg_node_set_justify_content(stat, YG_JUSTIFY_CENTER);
        yg_node_set_align_items(stat, YG_ALIGN_CENTER);
        yg_node_set_measure_func(stat, complex_measure, (void *)stats[i]);
        yg_node_add_child(row_horz, stat);
    }

    yg_node_t *list_section = yg_node_create();
    yg_node_set_flex_grow(list_section, 1.0f);
    yg_node_set_flex_direction(list_section, YG_FLEX_DIRECTION_COLUMN);
    yg_node_add_child(scroll_area, list_section);

    const char *feed[] = {"Recent Activity", "Notification: New message", "Update available",
                          "Friend request", "System alert", "Daily summary",
                          "Event reminder", "Promotion: 20% off"};
    for (int i = 0; i < 8; i++) {
        yg_node_t *row = yg_node_create();
        yg_node_set_height(row, 48.0f, YG_UNIT_POINT);
        yg_node_set_margin(row, YG_EDGE_BOTTOM, 4.0f, YG_UNIT_POINT);
        yg_node_set_padding(row, YG_EDGE_LEFT, 12.0f, YG_UNIT_POINT);
        yg_node_set_justify_content(row, YG_JUSTIFY_CENTER);
        yg_node_set_measure_func(row, complex_measure, (void *)feed[i]);
        yg_node_add_child(list_section, row);
    }

    yg_node_t *tabbar = yg_node_create();
    yg_node_set_height(tabbar, 56.0f, YG_UNIT_POINT);
    yg_node_set_flex_direction(tabbar, YG_FLEX_DIRECTION_ROW);
    yg_node_set_justify_content(tabbar, YG_JUSTIFY_SPACE_AROUND);
    yg_node_set_align_items(tabbar, YG_ALIGN_CENTER);
    yg_node_set_padding(tabbar, YG_EDGE_TOP, 4.0f, YG_UNIT_POINT);
    yg_node_add_child(page, tabbar);

    const char *tabs[] = {"Feed", "Search", "Post", "Alerts", "Me"};
    for (int i = 0; i < 5; i++) {
        yg_node_t *tab = yg_node_create();
        yg_node_set_width(tab, 48.0f, YG_UNIT_POINT);
        yg_node_set_height(tab, 44.0f, YG_UNIT_POINT);
        yg_node_set_justify_content(tab, YG_JUSTIFY_CENTER);
        yg_node_set_align_items(tab, YG_ALIGN_CENTER);
        yg_node_set_measure_func(tab, complex_measure, (void *)tabs[i]);
        yg_node_add_child(tabbar, tab);
    }

    yg_node_calculate_layout(page, 414.0f, 896.0f, YG_DIRECTION_LTR);

    printf("  Page layout: %.1f x %.1f (children=%d)\n",
           page->layout.width, page->layout.height, yg_node_child_count(page));
    printf("  Navbar:      %.1f x %.1f at (%.1f, %.1f)\n",
           navbar->layout.width, navbar->layout.height, navbar->layout.x, navbar->layout.y);
    printf("  Scroll area: %.1f x %.1f at (%.1f, %.1f)\n",
           scroll_area->layout.width, scroll_area->layout.height,
           scroll_area->layout.x, scroll_area->layout.y);
    printf("  Hero:        %.1f x %.1f at (%.1f, %.1f)\n",
           hero->layout.width, hero->layout.height, hero->layout.x, hero->layout.y);
    printf("  Tab bar:     %.1f x %.1f at (%.1f, %.1f)\n",
           tabbar->layout.width, tabbar->layout.height, tabbar->layout.x, tabbar->layout.y);

    yg_node_destroy(page);
}

static void demo_section_3(void)
{
    printf("\n===== SECTION 3 — VIRTUAL LIST + PUSH NOTIFICATIONS ====\n\n");

    lv_controller_t list;
    lv_init(&list);

    for (int i = 0; i < 200; i++) {
        cell_heights[i] = 44.0f + (float)((i * 17 + 7) % 60);
        snprintf(cell_label_buf[i], sizeof(cell_label_buf[i]),
                 "Contact %03d", i + 1);
        cell_labels[i] = cell_label_buf[i];
    }

    lv_set_item_count(&list, 200);
    lv_set_measure_fn(&list, lv_height_for_index, NULL);
    lv_set_render_fn(&list, lv_do_render, NULL);
    lv_set_scroll_fn(&list, lv_scroll_cb, NULL);
    lv_set_refresh_fn(&list, lv_refresh_cb, NULL);
    lv_set_end_reached_fn(&list, lv_end_cb, NULL);
    lv_set_press_fn(&list, lv_press_cb, NULL);

    lv_set_viewport(&list, 375.0f, 700.0f);
    lv_relayout(&list);

    printf("  List items: %d, visible: %d, first=%d, last=%d, height=%.1f\n",
           lv_get_item_count(&list), lv_get_visible_count(&list),
           lv_get_first_visible(&list), lv_get_last_visible(&list),
           list.content_height);

    printf("  Scroll tests:\n");
    lv_set_scroll_offset(&list, 500.0f);
    printf("    offset=500   => first=%d last=%d visible=%d\n",
           lv_get_first_visible(&list), lv_get_last_visible(&list),
           lv_get_visible_count(&list));

    lv_set_scroll_offset(&list, 2000.0f);
    printf("    offset=2000  => first=%d last=%d visible=%d\n",
           lv_get_first_visible(&list), lv_get_last_visible(&list),
           lv_get_visible_count(&list));

    lv_scroll_to_index(&list, 120, false);
    printf("    scroll to 120 => first=%d last=%d\n",
           lv_get_first_visible(&list), lv_get_last_visible(&list));

    lv_load_more_done(&list, 30);
    printf("    loaded 30 more => total=%d\n", lv_get_item_count(&list));

    printf("    index_at_point(10, 420) => %d\n", lv_index_for_point(&list, 10.0f, 420.0f));

    int sec_id = lv_add_section(&list, 0, 50, "Section Alpha", "End Alpha");
    printf("    Section added: id=%d\n", sec_id);

    lv_destroy(&list);

    printf("\n  Push notification tests:\n");
    pn_manager_t push;
    pn_init(&push);

    pn_set_token_callback(&push, token_callback, NULL);
    pn_set_receive_callback(&push, recv_callback, NULL);
    pn_set_action_callback(&push, act_callback, NULL);
    pn_set_deep_link_callback(&push, dl_callback, NULL);

    pn_register_device(&push, PN_SERVICE_APNS);

    pn_payload_t pl;
    pn_build_payload(&pl, "Welcome!", "Thanks for installing our app.",
                      "{\"screen\":\"home\"}");
    pn_set_badge(&pl, 1);
    pn_add_action(&pl, "open", "Open", false, true);
    pn_send_local(&push, &pl, 0.0f);

    pn_payload_t pl2;
    pn_build_payload(&pl2, "New Message", "Alice sent you a photo.",
                      "{\"chat_id\":\"alice\",\"type\":\"photo\"}");
    pn_set_badge(&pl2, 2);
    pn_add_action(&pl2, "reply", "Reply", false, true);
    pn_add_action(&pl2, "like", "Like", false, false);
    pn_send_local(&push, &pl2, 0.0f);

    pn_payload_t pl3;
    pn_build_payload(&pl3, NULL, NULL, "{\"sync\":\"all\",\"version\":5}");
    pn_set_silent(&pl3, true);
    pn_send_local(&push, &pl3, 0.0f);

    pn_set_badge_count(&push, 2);
    printf("    Badge count: %d\n", pn_get_badge_count(&push));
    printf("    Pending notifications: %d\n", pn_pending_count(&push));

    for (int i = 0; i < pn_pending_count(&push); i++) {
        const pn_notification_t *np = pn_get_pending(&push, i);
        if (np) {
            printf("      #%d: [%s] %s (silent=%d, link=%s)\n",
                   i, np->payload.title[0] ? np->payload.title : "(silent)",
                   np->payload.body[0] ? np->payload.body : "(no body)",
                   (int)np->payload.silent,
                   np->deep_link[0] ? np->deep_link : "(none)");
        }
    }

    pn_clear_all(&push);
    pn_destroy(&push);
    printf("    All cleared. Pending=%d\n", pn_pending_count(&push));
}

int main(void)
{
    printf("========================================\n");
    printf("  mini-mobile-client Full Integration\n");
    printf("  Demo — All Modules Working Together\n");
    printf("========================================\n");

    demo_section_1();
    demo_section_2();
    demo_section_3();

    printf("\n========================================\n");
    printf("  All sections completed successfully\n");
    printf("========================================\n");
    return 0;
}
