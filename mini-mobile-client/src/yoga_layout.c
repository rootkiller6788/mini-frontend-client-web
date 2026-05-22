#include "yoga_layout.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define YG_UNDEFINED NAN

static int next_node_id = 1;

yg_node_t *yg_node_create(void)
{
    yg_node_t *node = (yg_node_t *)calloc(1, sizeof(yg_node_t));
    if (!node) return NULL;

    node->id               = next_node_id++;
    node->direction        = YG_DIRECTION_INHERIT;
    node->flex_direction   = YG_FLEX_DIRECTION_COLUMN;
    node->justify_content  = YG_JUSTIFY_FLEX_START;
    node->align_items      = YG_ALIGN_STRETCH;
    node->align_content    = YG_ALIGN_FLEX_START;
    node->flex_grow        = 0.0f;
    node->flex_shrink      = 1.0f;
    node->flex_basis       = yg_value_auto();
    node->align_self       = YG_ALIGN_AUTO;
    node->width            = yg_value_auto();
    node->height           = yg_value_auto();
    node->min_width        = yg_value_undefined();
    node->min_height       = yg_value_undefined();
    node->max_width        = yg_value_undefined();
    node->max_height       = yg_value_undefined();
    node->aspect_ratio     = YG_UNDEFINED;
    node->position_type    = YG_POSITION_RELATIVE;
    node->display          = YG_DISPLAY_FLEX;
    node->overflow         = YG_OVERFLOW_VISIBLE;

    for (int i = 0; i < 4; i++) {
        node->margin[i]       = yg_value_undefined();
        node->padding[i]      = yg_value_undefined();
        node->border[i]       = yg_value_undefined();
        node->position_val[i] = yg_value_undefined();
    }

    return node;
}

void yg_node_destroy(yg_node_t *node)
{
    if (!node) return;
    free(node);
}

void yg_node_add_child(yg_node_t *parent, yg_node_t *child)
{
    if (!parent || !child) return;
    if (parent->child_count >= YG_MAX_CHILDREN) return;

    if (child->parent) {
        yg_node_remove_child(child->parent, child);
    }

    parent->children[parent->child_count++] = child;
    child->parent = parent;
}

void yg_node_remove_child(yg_node_t *parent, yg_node_t *child)
{
    if (!parent || !child) return;

    for (int i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            for (int j = i; j < parent->child_count - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->child_count--;
            child->parent = NULL;
            return;
        }
    }
}

int yg_node_child_count(yg_node_t *node)
{
    return node ? node->child_count : 0;
}

yg_node_t *yg_node_get_child(yg_node_t *node, int index)
{
    if (!node || index < 0 || index >= node->child_count) return NULL;
    return node->children[index];
}

static float resolve_value(yg_value_t val, float parent_size)
{
    if (val.unit == YG_UNIT_UNDEFINED) return YG_UNDEFINED;
    if (val.unit == YG_UNIT_AUTO)      return YG_UNDEFINED;
    if (val.unit == YG_UNIT_POINT)     return val.value;
    if (val.unit == YG_UNIT_PERCENT)   return parent_size * val.value * 0.01f;
    return YG_UNDEFINED;
}

static float clamp_val(float val, float min, float max)
{
    if (!isnan(min) && val < min) val = min;
    if (!isnan(max) && val > max) val = max;
    return val;
}

static void layout_node(yg_node_t *node, float parent_width, float parent_height,
                         yg_direction_t direction)
{
    (void)direction;
    if (!node || node->display == YG_DISPLAY_NONE) {
        if (node) {
            node->layout.width  = 0.0f;
            node->layout.height = 0.0f;
        }
        return;
    }

    if (node->measure && node->child_count == 0) {
        yg_measure_result_t mr = node->measure(node->measure_ctx,
            resolve_value(node->width, parent_width),
            node->width.unit,
            resolve_value(node->height, parent_height),
            node->height.unit);
        node->layout.width  = mr.width;
        node->layout.height = mr.height;
        return;
    }

    float avail_w = resolve_value(node->width, parent_width);
    float avail_h = resolve_value(node->height, parent_height);

    if (isnan(avail_w)) avail_w = parent_width;
    if (isnan(avail_h)) avail_h = parent_height;

    float total_pad_w = 0.0f, total_pad_h = 0.0f;
    for (int i = 0; i < 4; i++) {
        if (node->padding[i].unit == YG_UNIT_POINT) {
            if (i == YG_EDGE_LEFT || i == YG_EDGE_RIGHT)
                total_pad_w += node->padding[i].value;
            else
                total_pad_h += node->padding[i].value;
        }
        if (node->border[i].unit == YG_UNIT_POINT) {
            if (i == YG_EDGE_LEFT || i == YG_EDGE_RIGHT)
                total_pad_w += node->border[i].value;
            else
                total_pad_h += node->border[i].value;
        }
    }

    float content_w = avail_w - total_pad_w;
    float content_h = avail_h - total_pad_h;

    if (content_w < 0.0f) content_w = 0.0f;
    if (content_h < 0.0f) content_h = 0.0f;

    int abs_count = 0;
    for (int i = 0; i < node->child_count; i++) {
        if (node->children[i]->position_type == YG_POSITION_ABSOLUTE) abs_count++;
    }

    int flex_children = node->child_count - abs_count;
    float total_flex = 0.0f;
    float used_cross = 0.0f;
    bool is_row = (node->flex_direction == YG_FLEX_DIRECTION_ROW ||
                   node->flex_direction == YG_FLEX_DIRECTION_ROW_REVERSE);

    for (int i = 0; i < node->child_count; i++) {
        yg_node_t *child = node->children[i];
        if (child->position_type == YG_POSITION_ABSOLUTE || child->display == YG_DISPLAY_NONE)
            continue;
        total_flex += child->flex_grow;
    }

    float remaining = is_row ? content_w : content_h;
    if (flex_children > 0 && total_flex <= 0.0f) total_flex = (float)flex_children;

    for (int i = 0; i < node->child_count; i++) {
        yg_node_t *child = node->children[i];
        if (child->display == YG_DISPLAY_NONE) continue;

        if (child->position_type == YG_POSITION_ABSOLUTE) {
            float ax = resolve_value(child->position_val[YG_EDGE_LEFT], avail_w);
            float ay = resolve_value(child->position_val[YG_EDGE_TOP], avail_h);
            if (isnan(ax)) ax = 0.0f;
            if (isnan(ay)) ay = 0.0f;

            layout_node(child, avail_w, avail_h, node->direction);
            child->layout.x = ax;
            child->layout.y = ay;
            continue;
        }

        float child_main = 0.0f;
        if (node->flex_direction == YG_FLEX_DIRECTION_COLUMN ||
            node->flex_direction == YG_FLEX_DIRECTION_COLUMN_REVERSE) {

            float child_w = resolve_value(child->width, content_w);
            if (isnan(child_w)) child_w = content_w;

            float child_h = resolve_value(child->height, content_h);
            if (isnan(child_h)) {
                if (total_flex > 0.0f) {
                    child_h = (remaining / total_flex) * child->flex_grow;
                } else {
                    child_h = 0.0f;
                }
            }

            float bv = resolve_value(child->flex_basis, content_h);
            if (!isnan(bv)) child_h = bv;

            child_h = clamp_val(child_h,
                resolve_value(child->min_height, content_h),
                resolve_value(child->max_height, content_h));

            remaining -= child_h;
            if (child->flex_grow > 0.0f && total_flex > 0.0f) {
                remaining += child_h;
            }

            layout_node(child, child_w, child_h, node->direction);

            child->layout.x = 0.0f;
            child->layout.y = used_cross;
            used_cross += child_h;
            child_main = child_h;
        } else {
            float child_w = resolve_value(child->width, content_w);
            if (isnan(child_w)) {
                if (total_flex > 0.0f) {
                    child_w = (remaining / total_flex) * child->flex_grow;
                } else {
                    child_w = 0.0f;
                }
            }

            float bv = resolve_value(child->flex_basis, content_w);
            if (!isnan(bv)) child_w = bv;

            child_w = clamp_val(child_w,
                resolve_value(child->min_width, content_w),
                resolve_value(child->max_width, content_w));

            float child_h = resolve_value(child->height, content_h);
            if (isnan(child_h)) child_h = content_h;

            remaining -= child_w;
            if (child->flex_grow > 0.0f && total_flex > 0.0f) {
                remaining += child_w;
            }

            layout_node(child, child_w, child_h, node->direction);

            child->layout.y = 0.0f;
            child->layout.x = used_cross;
            used_cross += child_w;
            child_main = child_w;
        }

        (void)child_main;
    }

    float max_cross = used_cross;

    float justify_offset = 0.0f;
    if (flex_children > 1) {
        float space = remaining;
        if (space > 0.0f) {
            switch (node->justify_content) {
            case YG_JUSTIFY_CENTER:
                justify_offset = space * 0.5f;
                break;
            case YG_JUSTIFY_FLEX_END:
                justify_offset = space;
                break;
            case YG_JUSTIFY_SPACE_BETWEEN:
                justify_offset = 0.0f;
                break;
            case YG_JUSTIFY_SPACE_AROUND:
                justify_offset = space * 0.5f / (float)flex_children;
                break;
            case YG_JUSTIFY_SPACE_EVENLY:
                justify_offset = space / (float)(flex_children + 1);
                break;
            default:
                break;
            }
        }
    }

    if (justify_offset > 0.0f && node->justify_content == YG_JUSTIFY_CENTER) {
        for (int i = 0; i < node->child_count; i++) {
            yg_node_t *child = node->children[i];
            if (child->position_type == YG_POSITION_ABSOLUTE) continue;
            if (is_row) child->layout.x += justify_offset;
            else        child->layout.y += justify_offset;
        }
    }

    node->layout.width  = is_row ? max_cross + total_pad_w : avail_w;
    node->layout.height = is_row ? avail_h : used_cross + total_pad_h;

    if (is_row) node->layout.width = clamp_val(node->layout.width,
        resolve_value(node->min_width, parent_width),
        resolve_value(node->max_width, parent_width));
    else node->layout.height = clamp_val(node->layout.height,
        resolve_value(node->min_height, parent_height),
        resolve_value(node->max_height, parent_height));
}

void yg_node_calculate_layout(yg_node_t *node, float available_width,
                               float available_height, yg_direction_t direction)
{
    if (!node) return;
    layout_node(node, available_width, available_height, direction);
}

yg_rect_t yg_node_get_layout(yg_node_t *node)
{
    yg_rect_t r = {0, 0, 0, 0};
    if (node) r = node->layout;
    return r;
}

void yg_node_set_flex_direction(yg_node_t *node, yg_flex_direction_t val)
{ if (node) node->flex_direction = val; }

void yg_node_set_justify_content(yg_node_t *node, yg_justify_t val)
{ if (node) node->justify_content = val; }

void yg_node_set_align_items(yg_node_t *node, yg_align_t val)
{ if (node) node->align_items = val; }

void yg_node_set_align_content(yg_node_t *node, yg_align_t val)
{ if (node) node->align_content = val; }

void yg_node_set_align_self(yg_node_t *node, yg_align_t val)
{ if (node) node->align_self = val; }

void yg_node_set_flex_grow(yg_node_t *node, float val)
{ if (node) node->flex_grow = val; }

void yg_node_set_flex_shrink(yg_node_t *node, float val)
{ if (node) node->flex_shrink = val; }

void yg_node_set_flex_basis(yg_node_t *node, float val, yg_unit_t unit)
{ if (node) { node->flex_basis.value = val; node->flex_basis.unit = unit; } }

void yg_node_set_width(yg_node_t *node, float val, yg_unit_t unit)
{ if (node) { node->width.value = val; node->width.unit = unit; } }

void yg_node_set_height(yg_node_t *node, float val, yg_unit_t unit)
{ if (node) { node->height.value = val; node->height.unit = unit; } }

void yg_node_set_min_width(yg_node_t *node, float val, yg_unit_t unit)
{ if (node) { node->min_width.value = val; node->min_width.unit = unit; } }

void yg_node_set_min_height(yg_node_t *node, float val, yg_unit_t unit)
{ if (node) { node->min_height.value = val; node->min_height.unit = unit; } }

void yg_node_set_max_width(yg_node_t *node, float val, yg_unit_t unit)
{ if (node) { node->max_width.value = val; node->max_width.unit = unit; } }

void yg_node_set_max_height(yg_node_t *node, float val, yg_unit_t unit)
{ if (node) { node->max_height.value = val; node->max_height.unit = unit; } }

void yg_node_set_margin(yg_node_t *node, yg_edge_t edge, float val, yg_unit_t unit)
{
    if (!node) return;
    if (edge == YG_EDGE_ALL) {
        for (int i = 0; i < 4; i++) { node->margin[i].value = val; node->margin[i].unit = unit; }
    } else if (edge <= YG_EDGE_BOTTOM) {
        node->margin[edge].value = val; node->margin[edge].unit = unit;
    }
}

void yg_node_set_padding(yg_node_t *node, yg_edge_t edge, float val, yg_unit_t unit)
{
    if (!node) return;
    if (edge == YG_EDGE_ALL) {
        for (int i = 0; i < 4; i++) { node->padding[i].value = val; node->padding[i].unit = unit; }
    } else if (edge <= YG_EDGE_BOTTOM) {
        node->padding[edge].value = val; node->padding[edge].unit = unit;
    }
}

void yg_node_set_border(yg_node_t *node, yg_edge_t edge, float val)
{
    if (!node) return;
    if (edge == YG_EDGE_ALL) {
        for (int i = 0; i < 4; i++) { node->border[i].value = val; node->border[i].unit = YG_UNIT_POINT; }
    } else if (edge <= YG_EDGE_BOTTOM) {
        node->border[edge].value = val; node->border[edge].unit = YG_UNIT_POINT;
    }
}

void yg_node_set_position(yg_node_t *node, yg_edge_t edge, float val, yg_unit_t unit)
{
    if (!node) return;
    if (edge == YG_EDGE_ALL) {
        for (int i = 0; i < 4; i++) { node->position_val[i].value = val; node->position_val[i].unit = unit; }
    } else if (edge <= YG_EDGE_BOTTOM) {
        node->position_val[edge].value = val; node->position_val[edge].unit = unit;
    }
}

void yg_node_set_position_type(yg_node_t *node, yg_position_t val)
{ if (node) node->position_type = val; }

void yg_node_set_display(yg_node_t *node, yg_display_t val)
{ if (node) node->display = val; }

void yg_node_set_overflow(yg_node_t *node, yg_overflow_t val)
{ if (node) node->overflow = val; }

void yg_node_set_aspect_ratio(yg_node_t *node, float val)
{ if (node) node->aspect_ratio = val; }

void yg_node_set_measure_func(yg_node_t *node, yg_measure_fn fn, void *ctx)
{ if (node) { node->measure = fn; node->measure_ctx = ctx; } }

void yg_node_set_user_data(yg_node_t *node, void *data)
{ if (node) node->user_data = data; }

void *yg_node_get_user_data(yg_node_t *node)
{ return node ? node->user_data : NULL; }

yg_value_t yg_value_percent(float val)  { yg_value_t v = {val, YG_UNIT_PERCENT}; return v; }
yg_value_t yg_value_point(float val)    { yg_value_t v = {val, YG_UNIT_POINT}; return v; }
yg_value_t yg_value_auto(void)          { yg_value_t v = {0.0f, YG_UNIT_AUTO}; return v; }
yg_value_t yg_value_undefined(void)     { yg_value_t v = {0.0f, YG_UNIT_UNDEFINED}; return v; }

const char *yg_node_to_json(yg_node_t *node, char *buf, size_t len)
{
    if (!node || !buf || len < 64) return NULL;
    snprintf(buf, len,
        "{\"id\":%d,\"layout\":{\"x\":%.1f,\"y\":%.1f,\"w\":%.1f,\"h\":%.1f},\"children\":%d}",
        node->id,
        node->layout.x, node->layout.y,
        node->layout.width, node->layout.height,
        node->child_count);
    return buf;
}
