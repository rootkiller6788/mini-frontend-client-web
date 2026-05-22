#include "yoga_layout.h"
#include <stdio.h>
#include <string.h>

static yg_measure_result_t text_measure(void *ctx, float width, yg_unit_t width_mode,
                                         float height, yg_unit_t height_mode)
{
    (void)width_mode; (void)height_mode;
    const char *text = (const char *)ctx;
    float len = text ? (float)strlen(text) : 0.0f;
    yg_measure_result_t mr;
    mr.width   = width  > 0.0f ? (len * 8.0f > width ? width : len * 8.0f) : len * 8.0f;
    mr.height  = height > 0.0f ? height : 20.0f;
    mr.measured = true;
    return mr;
}

int main(void)
{
    yg_node_t *root = yg_node_create();
    yg_node_set_flex_direction(root, YG_FLEX_DIRECTION_COLUMN);
    yg_node_set_justify_content(root, YG_JUSTIFY_FLEX_START);
    yg_node_set_align_items(root, YG_ALIGN_STRETCH);
    yg_node_set_width(root, 375.0f, YG_UNIT_POINT);
    yg_node_set_height(root, 812.0f, YG_UNIT_POINT);
    yg_node_set_padding(root, YG_EDGE_ALL, 16.0f, YG_UNIT_POINT);

    yg_node_t *header = yg_node_create();
    yg_node_set_height(header, 64.0f, YG_UNIT_POINT);
    yg_node_set_width(header, 100.0f, YG_UNIT_PERCENT);
    yg_node_set_justify_content(header, YG_JUSTIFY_CENTER);
    yg_node_set_align_items(header, YG_ALIGN_CENTER);
    yg_node_set_measure_func(header, text_measure, (void *)"Header");
    yg_node_add_child(root, header);

    yg_node_t *body = yg_node_create();
    yg_node_set_flex_direction(body, YG_FLEX_DIRECTION_ROW);
    yg_node_set_flex_grow(body, 1.0f);
    yg_node_set_justify_content(body, YG_JUSTIFY_SPACE_BETWEEN);
    yg_node_add_child(root, body);

    yg_node_t *sidebar = yg_node_create();
    yg_node_set_width(sidebar, 80.0f, YG_UNIT_POINT);
    yg_node_set_height(sidebar, 100.0f, YG_UNIT_PERCENT);
    yg_node_set_padding(sidebar, YG_EDGE_ALL, 8.0f, YG_UNIT_POINT);
    yg_node_set_measure_func(sidebar, text_measure, (void *)"Sidebar");
    yg_node_add_child(body, sidebar);

    yg_node_t *content = yg_node_create();
    yg_node_set_flex_grow(content, 1.0f);
    yg_node_set_flex_direction(content, YG_FLEX_DIRECTION_COLUMN);
    yg_node_set_padding(content, YG_EDGE_ALL, 12.0f, YG_UNIT_POINT);
    yg_node_set_measure_func(content, text_measure, (void *)"Content Area");
    yg_node_add_child(body, content);

    yg_node_t *item1 = yg_node_create();
    yg_node_set_height(item1, 48.0f, YG_UNIT_POINT);
    yg_node_set_margin(item1, YG_EDGE_BOTTOM, 8.0f, YG_UNIT_POINT);
    yg_node_set_measure_func(item1, text_measure, (void *)"Item 1");
    yg_node_add_child(content, item1);

    yg_node_t *item2 = yg_node_create();
    yg_node_set_height(item2, 48.0f, YG_UNIT_POINT);
    yg_node_set_margin(item2, YG_EDGE_BOTTOM, 8.0f, YG_UNIT_POINT);
    yg_node_set_measure_func(item2, text_measure, (void *)"Item 2");
    yg_node_add_child(content, item2);

    yg_node_t *abs_overlay = yg_node_create();
    yg_node_set_position_type(abs_overlay, YG_POSITION_ABSOLUTE);
    yg_node_set_position(abs_overlay, YG_EDGE_BOTTOM, 20.0f, YG_UNIT_POINT);
    yg_node_set_position(abs_overlay, YG_EDGE_RIGHT, 20.0f, YG_UNIT_POINT);
    yg_node_set_width(abs_overlay, 56.0f, YG_UNIT_POINT);
    yg_node_set_height(abs_overlay, 56.0f, YG_UNIT_POINT);
    yg_node_set_measure_func(abs_overlay, text_measure, (void *)"FAB");
    yg_node_add_child(root, abs_overlay);

    yg_node_calculate_layout(root, 375.0f, 812.0f, YG_DIRECTION_LTR);

    char json_buf[512];
    yg_node_to_json(root, json_buf, sizeof(json_buf));
    printf("Root layout: %s\n", json_buf);

    printf("\nLayout details:\n");
    printf("  root:     %.1f x %.1f at (%.1f, %.1f)  children=%d\n",
           root->layout.width, root->layout.height,
           root->layout.x, root->layout.y, yg_node_child_count(root));
    printf("  header:   %.1f x %.1f at (%.1f, %.1f)\n",
           header->layout.width, header->layout.height,
           header->layout.x, header->layout.y);
    printf("  body:     %.1f x %.1f at (%.1f, %.1f)\n",
           body->layout.width, body->layout.height,
           body->layout.x, body->layout.y);
    printf("  sidebar:  %.1f x %.1f at (%.1f, %.1f)\n",
           sidebar->layout.width, sidebar->layout.height,
           sidebar->layout.x, sidebar->layout.y);
    printf("  content:  %.1f x %.1f at (%.1f, %.1f)\n",
           content->layout.width, content->layout.height,
           content->layout.x, content->layout.y);
    printf("  item1:    %.1f x %.1f at (%.1f, %.1f)\n",
           item1->layout.width, item1->layout.height,
           item1->layout.x, item1->layout.y);
    printf("  item2:    %.1f x %.1f at (%.1f, %.1f)\n",
           item2->layout.width, item2->layout.height,
           item2->layout.x, item2->layout.y);
    printf("  abs FAB:  %.1f x %.1f at (%.1f, %.1f)\n",
           abs_overlay->layout.width, abs_overlay->layout.height,
           abs_overlay->layout.x, abs_overlay->layout.y);

    yg_node_destroy(root);
    printf("\nLayout nodes destroyed\n");
    return 0;
}
