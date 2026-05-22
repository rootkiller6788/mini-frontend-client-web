#include "layout_engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

LayoutBox* layout_box_create(void) {
    LayoutBox* box = (LayoutBox*)calloc(1, sizeof(LayoutBox));
    if (!box) return NULL;
    box->box = box_model_default();
    box->display = DISPLAY_BLOCK;
    box->position = POSITION_STATIC;
    box->flex_direction = FLEX_DIRECTION_ROW;
    box->flex_wrap = FLEX_WRAP_NOWRAP;
    box->justify_content = JUSTIFY_FLEX_START;
    box->align_items = ALIGN_STRETCH;
    return box;
}

void layout_box_free(LayoutBox* box) {
    if (!box) return;
    free(box);
}

void layout_box_free_recursive(LayoutBox* box) {
    if (!box) return;
    LayoutBox* child = box->first_child;
    while (child) {
        LayoutBox* next = child->next_sibling;
        layout_box_free_recursive(child);
        child = next;
    }
    layout_box_free(box);
}

void layout_box_append_child(LayoutBox* parent, LayoutBox* child) {
    if (!parent || !child) return;
    child->parent = parent;
    if (!parent->first_child) {
        parent->first_child = child;
        parent->last_child = child;
    } else {
        parent->last_child->next_sibling = child;
        child->prev_sibling = parent->last_child;
        parent->last_child = child;
    }
    parent->child_count++;
}

void layout_context_init(LayoutContext* ctx, float viewport_width, float viewport_height) {
    ctx->viewport_width = viewport_width;
    ctx->viewport_height = viewport_height;
    ctx->root_font_size = 16.0f;
    ctx->root_box = NULL;
}

void layout_context_free(LayoutContext* ctx) {
    if (ctx->root_box) {
        layout_box_free_recursive(ctx->root_box);
        ctx->root_box = NULL;
    }
}

BoxModel box_model_default(void) {
    BoxModel box;
    memset(&box, 0, sizeof(BoxModel));
    return box;
}

void box_model_compute_size(BoxModel* box, float available_width, float available_height) {
    if (!box) return;
    if (box->content_width <= 0 && available_width > 0) {
        box->content_width = available_width -
            box->margin_left - box->margin_right -
            box->border_left - box->border_right -
            box->padding_left - box->padding_right;
    }
    if (box->content_height <= 0 && available_height > 0) {
        box->content_height = available_height -
            box->margin_top - box->margin_bottom -
            box->border_top - box->border_bottom -
            box->padding_top - box->padding_bottom;
    }
    if (box->content_width < 0) box->content_width = 0;
    if (box->content_height < 0) box->content_height = 0;
}

float box_model_total_width(const BoxModel* box) {
    if (!box) return 0;
    return box->margin_left + box->border_left + box->padding_left +
           box->content_width +
           box->padding_right + box->border_right + box->margin_right;
}

float box_model_total_height(const BoxModel* box) {
    if (!box) return 0;
    return box->margin_top + box->border_top + box->padding_top +
           box->content_height +
           box->padding_bottom + box->border_bottom + box->margin_bottom;
}

float box_model_content_x(const BoxModel* box) {
    if (!box) return 0;
    return box->margin_left + box->border_left + box->padding_left;
}

float box_model_content_y(const BoxModel* box) {
    if (!box) return 0;
    return box->margin_top + box->border_top + box->padding_top;
}

static DisplayType parse_display(const char* value) {
    if (!value) return DISPLAY_BLOCK;
    if (strcmp(value, "none") == 0) return DISPLAY_NONE;
    if (strcmp(value, "inline") == 0) return DISPLAY_INLINE;
    if (strcmp(value, "inline-block") == 0) return DISPLAY_INLINE_BLOCK;
    if (strcmp(value, "flex") == 0) return DISPLAY_FLEX;
    if (strcmp(value, "inline-flex") == 0) return DISPLAY_INLINE_FLEX;
    return DISPLAY_BLOCK;
}

static PositionType parse_position(const char* value) {
    if (!value) return POSITION_STATIC;
    if (strcmp(value, "relative") == 0) return POSITION_RELATIVE;
    if (strcmp(value, "absolute") == 0) return POSITION_ABSOLUTE;
    if (strcmp(value, "fixed") == 0) return POSITION_FIXED;
    return POSITION_STATIC;
}

static float parse_css_value(const char* value, float parent_value, float root_font_size) {
    if (!value) return 0;
    char* end = NULL;
    float num = strtof(value, &end);
    if (end && end != value) {
        if (strstr(value, "px")) return num;
        if (strstr(value, "em")) return num * root_font_size;
        if (strstr(value, "rem")) return num * root_font_size;
        if (strstr(value, "%")) return num * parent_value / 100.0f;
        return num;
    }
    return 0;
}

static void apply_computed_style_to_layout(const DomNode* dom_node, LayoutBox* box, LayoutContext* ctx) {
    if (!dom_node || !box) return;

    const char* display_val = dom_node_get_computed_style(dom_node, "display");
    if (display_val) box->display = parse_display(display_val);

    const char* position_val = dom_node_get_computed_style(dom_node, "position");
    if (position_val) box->position = parse_position(position_val);

    float parent_w = ctx->viewport_width;
    float parent_h = ctx->viewport_height;

    const char* m = dom_node_get_computed_style(dom_node, "margin");
    const char* mt = dom_node_get_computed_style(dom_node, "margin-top");
    const char* mr = dom_node_get_computed_style(dom_node, "margin-right");
    const char* mb = dom_node_get_computed_style(dom_node, "margin-bottom");
    const char* ml = dom_node_get_computed_style(dom_node, "margin-left");

    float margin_val = m ? parse_css_value(m, parent_w, ctx->root_font_size) : 0;
    box->box.margin_top = mt ? parse_css_value(mt, parent_h, ctx->root_font_size) : margin_val;
    box->box.margin_right = mr ? parse_css_value(mr, parent_w, ctx->root_font_size) : margin_val;
    box->box.margin_bottom = mb ? parse_css_value(mb, parent_h, ctx->root_font_size) : margin_val;
    box->box.margin_left = ml ? parse_css_value(ml, parent_w, ctx->root_font_size) : margin_val;

    const char* p = dom_node_get_computed_style(dom_node, "padding");
    const char* pt = dom_node_get_computed_style(dom_node, "padding-top");
    const char* pr = dom_node_get_computed_style(dom_node, "padding-right");
    const char* pb = dom_node_get_computed_style(dom_node, "padding-bottom");
    const char* pl = dom_node_get_computed_style(dom_node, "padding-left");

    float pad_val = p ? parse_css_value(p, parent_w, ctx->root_font_size) : 0;
    box->box.padding_top = pt ? parse_css_value(pt, parent_h, ctx->root_font_size) : pad_val;
    box->box.padding_right = pr ? parse_css_value(pr, parent_w, ctx->root_font_size) : pad_val;
    box->box.padding_bottom = pb ? parse_css_value(pb, parent_h, ctx->root_font_size) : pad_val;
    box->box.padding_left = pl ? parse_css_value(pl, parent_w, ctx->root_font_size) : pad_val;

    const char* w = dom_node_get_computed_style(dom_node, "width");
    const char* h = dom_node_get_computed_style(dom_node, "height");
    if (w) box->box.content_width = parse_css_value(w, parent_w, ctx->root_font_size);
    if (h) box->box.content_height = parse_css_value(h, parent_h, ctx->root_font_size);

    box->width = box_model_total_width(&box->box);
    box->height = box_model_total_height(&box->box);
}

static LayoutBox* build_layout_tree_recursive(const DomNode* dom_node, LayoutContext* ctx, LayoutBox* parent) {
    if (!dom_node) return NULL;
    if (dom_node->type != DOM_NODE_ELEMENT && dom_node->type != DOM_NODE_DOCUMENT) return NULL;

    LayoutBox* box = layout_box_create();
    box->dom_node = (DomNode*)dom_node;

    const char* display_val = dom_node_get_computed_style(dom_node, "display");
    if (display_val) box->display = parse_display(display_val);

    apply_computed_style_to_layout(dom_node, box, ctx);

    if (box->display == DISPLAY_NONE) {
        layout_box_free(box);
        return NULL;
    }

    DomNode* child = dom_node->first_child;
    while (child) {
        LayoutBox* child_box = build_layout_tree_recursive(child, ctx, box);
        if (child_box) {
            layout_box_append_child(box, child_box);
        }
        child = child->next_sibling;
    }

    return box;
}

LayoutBox* layout_compute(const DomNode* dom_root, LayoutContext* ctx) {
    if (!dom_root || !ctx) return NULL;

    ctx->root_box = build_layout_tree_recursive(dom_root, ctx, NULL);
    if (!ctx->root_box) return NULL;

    layout_compute_box(ctx->root_box, ctx, ctx->viewport_width);
    return ctx->root_box;
}

void layout_compute_box(LayoutBox* box, LayoutContext* ctx, float available_width) {
    if (!box || !ctx) return;

    apply_computed_style_to_layout(box->dom_node, box, ctx);

    if (box->display == DISPLAY_FLEX || box->display == DISPLAY_INLINE_FLEX) {
        layout_compute_flex(box, ctx);
    } else {
        float y_offset = 0;
        LayoutBox* child = box->first_child;
        while (child) {
            if (child->position == POSITION_ABSOLUTE || child->position == POSITION_FIXED) {
                layout_compute_position(child, ctx);
                child = child->next_sibling;
                continue;
            }

            float child_avail_w = box->box.content_width;
            if (child_avail_w <= 0) child_avail_w = available_width;

            layout_compute_box(child, ctx, child_avail_w);

            child->x = box_model_content_x(&box->box);
            child->y = box_model_content_y(&box->box) + y_offset;

            if (box->display == DISPLAY_BLOCK) {
                y_offset += child->height;
            } else {
                y_offset = fmaxf(y_offset, child->height);
            }

            child = child->next_sibling;
        }

        if (box->display == DISPLAY_BLOCK) {
            if (box->width <= 0) box->width = available_width;
            if (box->height <= 0) box->height = y_offset + box->box.padding_bottom + box->box.border_bottom;
        }
    }

    box->laid_out = true;
}

void layout_compute_flex(LayoutBox* flex_box, LayoutContext* ctx) {
    if (!flex_box || !ctx) return;

    apply_computed_style_to_layout(flex_box->dom_node, flex_box, ctx);

    bool is_row = (flex_box->flex_direction == FLEX_DIRECTION_ROW ||
                   flex_box->flex_direction == FLEX_DIRECTION_ROW_REVERSE);

    float total_child_size = 0;
    size_t visible_children = 0;
    float max_cross_size = 0;

    LayoutBox* child = flex_box->first_child;
    while (child) {
        if (child->display != DISPLAY_NONE) {
            layout_compute_box(child, ctx, flex_box->box.content_width);
            total_child_size += is_row ? child->width : child->height;
            float cross_size = is_row ? child->height : child->width;
            if (cross_size > max_cross_size) max_cross_size = cross_size;
            visible_children++;
        }
        child = child->next_sibling;
    }

    float container_main = is_row ? flex_box->box.content_width : flex_box->box.content_height;
    float container_cross = is_row ? flex_box->box.content_height : flex_box->box.content_width;
    if (container_main <= 0) container_main = is_row ? ctx->viewport_width : ctx->viewport_height;

    float gap = 0;
    if (visible_children > 1) {
        switch (flex_box->justify_content) {
            case JUSTIFY_FLEX_END:
            case JUSTIFY_CENTER:
                gap = 0;
                break;
            case JUSTIFY_SPACE_BETWEEN:
                gap = (container_main - total_child_size) / (float)(visible_children - 1);
                break;
            case JUSTIFY_SPACE_AROUND:
                gap = (container_main - total_child_size) / (float)visible_children;
                break;
            case JUSTIFY_SPACE_EVENLY:
                gap = (container_main - total_child_size) / (float)(visible_children + 1);
                break;
            default:
                gap = 0;
                break;
        }
    }

    float main_offset = 0;
    switch (flex_box->justify_content) {
        case JUSTIFY_FLEX_END:
            main_offset = container_main - total_child_size;
            break;
        case JUSTIFY_CENTER:
            main_offset = (container_main - total_child_size) / 2.0f;
            break;
        case JUSTIFY_SPACE_AROUND:
            main_offset = gap / 2.0f;
            break;
        case JUSTIFY_SPACE_EVENLY:
            main_offset = gap;
            break;
        default:
            main_offset = 0;
            break;
    }

    child = flex_box->first_child;
    while (child) {
        if (child->display != DISPLAY_NONE) {
            if (is_row) {
                child->x = box_model_content_x(&flex_box->box) + main_offset;
                child->y = box_model_content_y(&flex_box->box);
                main_offset += child->width + gap;
            } else {
                child->x = box_model_content_x(&flex_box->box);
                child->y = box_model_content_y(&flex_box->box) + main_offset;
                main_offset += child->height + gap;
            }
        }
        child = child->next_sibling;
    }

    flex_box->width = box_model_total_width(&flex_box->box);
    flex_box->height = box_model_total_height(&flex_box->box);
    if (flex_box->width <= 0) flex_box->width = container_main + box_model_content_x(&flex_box->box) * 2;
    if (flex_box->height <= 0) flex_box->height = max_cross_size + box_model_content_y(&flex_box->box) * 2;

    flex_box->laid_out = true;
}

void layout_compute_position(LayoutBox* box, LayoutContext* ctx) {
    if (!box || !ctx) return;
    (void)ctx;

    const char* top_val = dom_node_get_computed_style(box->dom_node, "top");
    const char* right_val = dom_node_get_computed_style(box->dom_node, "right");
    const char* bottom_val = dom_node_get_computed_style(box->dom_node, "bottom");
    const char* left_val = dom_node_get_computed_style(box->dom_node, "left");

    if (top_val) box->y = parse_css_value(top_val, ctx->viewport_height, ctx->root_font_size);
    if (left_val) box->x = parse_css_value(left_val, ctx->viewport_width, ctx->root_font_size);
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

void layout_print_tree(const LayoutBox* root, int indent) {
    if (!root) return;
    print_indent(indent);

    const char* tag = root->dom_node && root->dom_node->tag_name ? root->dom_node->tag_name : "(anon)";
    const char* display_str = "block";
    switch (root->display) {
        case DISPLAY_FLEX: display_str = "flex"; break;
        case DISPLAY_INLINE: display_str = "inline"; break;
        case DISPLAY_INLINE_BLOCK: display_str = "inline-block"; break;
        case DISPLAY_NONE: display_str = "none"; break;
        default: break;
    }

    printf("<%s> display=%s pos=(%.0f,%.0f) size=%.0fx%.0f"
           " margin=(%.0f,%.0f,%.0f,%.0f) padding=(%.0f,%.0f,%.0f,%.0f)\n",
           tag, display_str,
           root->x, root->y, root->width, root->height,
           root->box.margin_top, root->box.margin_right,
           root->box.margin_bottom, root->box.margin_left,
           root->box.padding_top, root->box.padding_right,
           root->box.padding_bottom, root->box.padding_left);

    LayoutBox* child = root->first_child;
    while (child) {
        layout_print_tree(child, indent + 1);
        child = child->next_sibling;
    }
}
