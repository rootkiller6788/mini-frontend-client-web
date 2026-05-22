#include "react_native_bridge.h"
#include "native_modules.h"
#include "yoga_layout.h"
#include "list_virtualizer.h"
#include "push_notif.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static float demo_heights[100];

static yg_measure_result_t demo_text_measure(void *ctx, float width, yg_unit_t width_mode,
                                              float height, yg_unit_t height_mode)
{
    (void)width_mode; (void)height_mode;
    const char *label = (const char *)ctx;
    size_t len = label ? strlen(label) : 0;
    yg_measure_result_t mr;
    mr.width   = width > 0.0f ? width : (float)(len * 10.0f + 20.0f);
    mr.height  = height > 0.0f ? height : 36.0f;
    mr.measured = true;
    return mr;
}

static void demo_token_cb(const char *token, void *ctx)
{
    (void)ctx;
    printf("[Push] Device token received: %.40s...\n", token);
}

static void demo_receive_cb(const pn_notification_t *notif, pn_app_state_t state, void *ctx)
{
    (void)ctx;
    printf("[Push] Received: [%s] %s (app_state=%d)\n",
           notif->payload.title, notif->payload.body, (int)state);
}

static void demo_action_cb(const char *action_id, const pn_notification_t *notif, void *ctx)
{
    (void)ctx;
    printf("[Push] Action '%s' on '%s'\n", action_id, notif->payload.title);
}

static void demo_deeplink_cb(const char *link, void *ctx)
{
    (void)ctx;
    printf("[Push] Deep link: %s\n", link);
}

static float demo_list_measure(int index, lv_cell_type_t type, void *ctx)
{
    (void)type; (void)ctx;
    if (index < 100) return demo_heights[index];
    return 60.0f;
}

static lv_cell_t *demo_list_render(int index, lv_cell_t *recycled, void *ctx)
{
    (void)ctx;
    static lv_cell_t cell;
    memset(&cell, 0, sizeof(cell));
    cell.id      = index;
    cell.type    = LV_CELL_ITEM;
    cell.index   = index;
    cell.height  = demo_heights[index];
    cell.width   = 375.0f;
    cell.data    = &cell;
    cell.visible = true;
    (void)recycled;
    return &cell;
}

static void demo_list_scroll(float offset, lv_scroll_state_t state, void *ctx)
{
    (void)ctx;
    printf("[List] scroll offset=%.1f state=%d\n", offset, (int)state);
}

static void demo_list_refresh(void *ctx)
{
    (void)ctx;
    printf("[List] Pull-to-refresh triggered!\n");
}

static void demo_list_end_reached(void *ctx)
{
    (void)ctx;
    printf("[List] End reached (load more)...\n");
}

static void demo_list_press(int index, void *ctx)
{
    (void)ctx;
    printf("[List] Cell %d pressed\n", index);
}

int main(void)
{
    printf("=== mini-mobile-client Basic Demo ===\n\n");

    printf("--- 1. React Native Bridge ---\n");
    rn_bridge_t bridge;
    rn_bridge_init(&bridge);
    rn_bridge_send_message(&bridge, RN_THREAD_JS, "App", "init", "{\"version\":\"1.0\"}");
    printf("Bridge pending: %d messages\n", rn_bridge_pending_count(&bridge));
    rn_bridge_flush(&bridge, RN_THREAD_JS);
    printf("After flush: %d messages\n", rn_bridge_pending_count(&bridge));
    rn_bridge_destroy(&bridge);

    printf("\n--- 2. Native Modules ---\n");
    nm_registry_t reg;
    nm_registry_init(&reg);

    nm_set_permission(&reg, NM_PERM_STORAGE, NM_PERM_GRANTED);
    nm_set_permission(&reg, NM_PERM_LOCATION, NM_PERM_GRANTED);
    nm_set_permission(&reg, NM_PERM_CAMERA, NM_PERM_GRANTED);

    printf("Permissions: Camera=%d Location=%d Storage=%d Mic=%d\n",
           nm_check_permission(&reg, NM_PERM_CAMERA),
           nm_check_permission(&reg, NM_PERM_LOCATION),
           nm_check_permission(&reg, NM_PERM_STORAGE),
           nm_check_permission(&reg, NM_PERM_MIC));

    nm_registry_destroy(&reg);

    printf("\n--- 3. Yoga Layout Engine ---\n");
    yg_node_t *screen = yg_node_create();
    yg_node_set_width(screen, 375.0f, YG_UNIT_POINT);
    yg_node_set_height(screen, 812.0f, YG_UNIT_POINT);
    yg_node_set_flex_direction(screen, YG_FLEX_DIRECTION_COLUMN);
    yg_node_set_padding(screen, YG_EDGE_ALL, 16.0f, YG_UNIT_POINT);

    yg_node_t *top_bar = yg_node_create();
    yg_node_set_height(top_bar, 44.0f, YG_UNIT_POINT);
    yg_node_set_width(top_bar, 100.0f, YG_UNIT_PERCENT);
    yg_node_set_justify_content(top_bar, YG_JUSTIFY_CENTER);
    yg_node_set_align_items(top_bar, YG_ALIGN_CENTER);
    yg_node_set_measure_func(top_bar, demo_text_measure, (void *)"App Title");
    yg_node_add_child(screen, top_bar);

    yg_node_t *main_content = yg_node_create();
    yg_node_set_flex_grow(main_content, 1.0f);
    yg_node_set_flex_direction(main_content, YG_FLEX_DIRECTION_COLUMN);
    yg_node_set_justify_content(main_content, YG_JUSTIFY_FLEX_START);
    yg_node_set_padding(main_content, YG_EDGE_ALL, 8.0f, YG_UNIT_POINT);
    yg_node_add_child(screen, main_content);

    yg_node_t *cards[5];
    const char *card_labels[] = {"Dashboard", "Messages", "Profile", "Settings", "About"};
    for (int i = 0; i < 5; i++) {
        cards[i] = yg_node_create();
        yg_node_set_height(cards[i], 56.0f, YG_UNIT_POINT);
        yg_node_set_margin(cards[i], YG_EDGE_BOTTOM, 8.0f, YG_UNIT_POINT);
        yg_node_set_padding(cards[i], YG_EDGE_ALL, 12.0f, YG_UNIT_POINT);
        yg_node_set_justify_content(cards[i], YG_JUSTIFY_CENTER);
        yg_node_set_measure_func(cards[i], demo_text_measure, (void *)card_labels[i]);
        yg_node_add_child(main_content, cards[i]);
    }

    yg_node_t *bottom_tab = yg_node_create();
    yg_node_set_height(bottom_tab, 49.0f, YG_UNIT_POINT);
    yg_node_set_width(bottom_tab, 100.0f, YG_UNIT_PERCENT);
    yg_node_set_flex_direction(bottom_tab, YG_FLEX_DIRECTION_ROW);
    yg_node_set_justify_content(bottom_tab, YG_JUSTIFY_SPACE_AROUND);
    yg_node_set_align_items(bottom_tab, YG_ALIGN_CENTER);
    yg_node_set_padding(bottom_tab, YG_EDGE_TOP, 4.0f, YG_UNIT_POINT);
    yg_node_add_child(screen, bottom_tab);

    const char *tab_labels[] = {"Home", "Search", "Add", "Notifications", "Profile"};
    for (int i = 0; i < 5; i++) {
        yg_node_t *tab = yg_node_create();
        yg_node_set_width(tab, 60.0f, YG_UNIT_POINT);
        yg_node_set_height(tab, 44.0f, YG_UNIT_POINT);
        yg_node_set_justify_content(tab, YG_JUSTIFY_CENTER);
        yg_node_set_align_items(tab, YG_ALIGN_CENTER);
        yg_node_set_measure_func(tab, demo_text_measure, (void *)tab_labels[i]);
        yg_node_add_child(bottom_tab, tab);
    }

    yg_node_t *abs_badge = yg_node_create();
    yg_node_set_position_type(abs_badge, YG_POSITION_ABSOLUTE);
    yg_node_set_position(abs_badge, YG_EDGE_TOP, 50.0f, YG_UNIT_POINT);
    yg_node_set_position(abs_badge, YG_EDGE_RIGHT, 24.0f, YG_UNIT_POINT);
    yg_node_set_width(abs_badge, 24.0f, YG_UNIT_POINT);
    yg_node_set_height(abs_badge, 24.0f, YG_UNIT_POINT);
    yg_node_set_measure_func(abs_badge, demo_text_measure, (void *)"99");
    yg_node_add_child(screen, abs_badge);

    yg_node_calculate_layout(screen, 375.0f, 812.0f, YG_DIRECTION_LTR);
    printf("Screen layout: %.1fx%.1f with %d children\n",
           screen->layout.width, screen->layout.height, yg_node_child_count(screen));
    printf("  Top bar:     %.1fx%.1f at (%.1f, %.1f)\n",
           top_bar->layout.width, top_bar->layout.height,
           top_bar->layout.x, top_bar->layout.y);
    printf("  Main content: %.1fx%.1f at (%.1f, %.1f)\n",
           main_content->layout.width, main_content->layout.height,
           main_content->layout.x, main_content->layout.y);
    printf("  Bottom tab:  %.1fx%.1f at (%.1f, %.1f)\n",
           bottom_tab->layout.width, bottom_tab->layout.height,
           bottom_tab->layout.x, bottom_tab->layout.y);
    printf("  Badge:       %.1fx%.1f at (%.1f, %.1f)\n",
           abs_badge->layout.width, abs_badge->layout.height,
           abs_badge->layout.x, abs_badge->layout.y);

    yg_node_destroy(screen);

    printf("\n--- 4. List Virtualizer ---\n");
    lv_controller_t list;
    lv_init(&list);

    for (int i = 0; i < 100; i++) {
        demo_heights[i] = 40.0f + (float)((i * 13) % 80);
    }

    lv_set_item_count(&list, 100);
    lv_set_measure_fn(&list, demo_list_measure, NULL);
    lv_set_render_fn(&list, demo_list_render, NULL);
    lv_set_scroll_fn(&list, demo_list_scroll, NULL);
    lv_set_refresh_fn(&list, demo_list_refresh, NULL);
    lv_set_end_reached_fn(&list, demo_list_end_reached, NULL);
    lv_set_press_fn(&list, demo_list_press, NULL);

    lv_set_viewport(&list, 375.0f, 600.0f);
    lv_relayout(&list);

    printf("Items: %d, Visible: %d (first=%d, last=%d), Content height: %.1f\n",
           lv_get_item_count(&list),
           lv_get_visible_count(&list),
           lv_get_first_visible(&list),
           lv_get_last_visible(&list),
           list.content_height);

    lv_set_scroll_offset(&list, 350.0f);
    printf("Scrolled to 350, first visible: %d\n", lv_get_first_visible(&list));

    lv_set_scroll_offset(&list, 2400.0f);
    printf("Scrolled near end, end reached check triggered\n");

    lv_scroll_to_index(&list, 50, false);
    printf("Jumped to index 50\n");

    lv_load_more_done(&list, 20);
    printf("20 more items loaded, total: %d\n", lv_get_item_count(&list));

    lv_destroy(&list);

    printf("\n--- 5. Push Notifications ---\n");
    pn_manager_t push;
    pn_init(&push);

    pn_set_token_callback(&push, demo_token_cb, NULL);
    pn_set_receive_callback(&push, demo_receive_cb, NULL);
    pn_set_action_callback(&push, demo_action_cb, NULL);
    pn_set_deep_link_callback(&push, demo_deeplink_cb, NULL);

    pn_register_device(&push, PN_SERVICE_FCM);
    printf("Device registered: %s (token=%.32s...)\n",
           push.registered ? "yes" : "no", push.device_token);

    pn_payload_t payload;
    pn_build_payload(&payload, "Hello", "You have a new message!", "{\"chat_id\":\"42\"}");
    pn_set_badge(&payload, 5);
    pn_add_action(&payload, "reply", "Reply", false, true);
    pn_add_action(&payload, "dismiss", "Dismiss", true, false);

    pn_notification_t notif;
    memset(&notif, 0, sizeof(notif));
    notif.id = 1;
    memcpy(&notif.payload, &payload, sizeof(payload));
    pn_set_deep_link(&notif, "myapp://chat/42");

    pn_deliver_notification(&push, &notif, PN_STATE_ACTIVE);
    printf("Local notification sent\n");

    pn_set_badge_count(&push, 5);
    printf("Badge count: %d\n", pn_get_badge_count(&push));
    printf("Pending: %d\n", pn_pending_count(&push));

    pn_payload_t silent_payload;
    pn_build_payload(&silent_payload, NULL, NULL, "{\"sync\":\"contacts\"}");
    pn_set_silent(&silent_payload, true);

    pn_notification_t silent_notif;
    memset(&silent_notif, 0, sizeof(silent_notif));
    silent_notif.id = 2;
    memcpy(&silent_notif.payload, &silent_payload, sizeof(silent_payload));
    silent_notif.payload.silent = true;

    pn_deliver_notification(&push, &silent_notif, PN_STATE_ACTIVE);
    printf("Silent push sent\n");

    pn_clear_all(&push);
    printf("All notifications cleared. Pending: %d, Badge: %d\n",
           pn_pending_count(&push), pn_get_badge_count(&push));

    pn_destroy(&push);

    printf("\n=== Demo Complete ===\n");
    return 0;
}
