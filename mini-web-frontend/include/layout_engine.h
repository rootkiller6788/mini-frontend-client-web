#ifndef LAYOUT_ENGINE_H
#define LAYOUT_ENGINE_H

#include <stdbool.h>
#include <stddef.h>

#include "dom_tree.h"

typedef enum {
    DISPLAY_BLOCK,
    DISPLAY_INLINE,
    DISPLAY_INLINE_BLOCK,
    DISPLAY_FLEX,
    DISPLAY_INLINE_FLEX,
    DISPLAY_NONE
} DisplayType;

typedef enum {
    POSITION_STATIC,
    POSITION_RELATIVE,
    POSITION_ABSOLUTE,
    POSITION_FIXED
} PositionType;

typedef enum {
    FLEX_DIRECTION_ROW,
    FLEX_DIRECTION_ROW_REVERSE,
    FLEX_DIRECTION_COLUMN,
    FLEX_DIRECTION_COLUMN_REVERSE
} FlexDirection;

typedef enum {
    FLEX_WRAP_NOWRAP,
    FLEX_WRAP_WRAP,
    FLEX_WRAP_WRAP_REVERSE
} FlexWrap;

typedef enum {
    JUSTIFY_FLEX_START,
    JUSTIFY_FLEX_END,
    JUSTIFY_CENTER,
    JUSTIFY_SPACE_BETWEEN,
    JUSTIFY_SPACE_AROUND,
    JUSTIFY_SPACE_EVENLY
} JustifyContent;

typedef enum {
    ALIGN_FLEX_START,
    ALIGN_FLEX_END,
    ALIGN_CENTER,
    ALIGN_STRETCH,
    ALIGN_BASELINE
} AlignItems;

typedef struct {
    float margin_top;
    float margin_right;
    float margin_bottom;
    float margin_left;
    float border_top;
    float border_right;
    float border_bottom;
    float border_left;
    float padding_top;
    float padding_right;
    float padding_bottom;
    float padding_left;
    float content_width;
    float content_height;
} BoxModel;

typedef struct LayoutBox {
    BoxModel box;
    float x;
    float y;
    float width;
    float height;
    PositionType position;
    DisplayType display;
    FlexDirection flex_direction;
    FlexWrap flex_wrap;
    JustifyContent justify_content;
    AlignItems align_items;
    bool laid_out;
    DomNode* dom_node;
    struct LayoutBox* parent;
    struct LayoutBox* first_child;
    struct LayoutBox* last_child;
    struct LayoutBox* next_sibling;
    struct LayoutBox* prev_sibling;
    size_t child_count;
} LayoutBox;

typedef struct {
    float viewport_width;
    float viewport_height;
    float root_font_size;
    LayoutBox* root_box;
} LayoutContext;

LayoutBox* layout_box_create(void);
void layout_box_free(LayoutBox* box);
void layout_box_free_recursive(LayoutBox* box);
void layout_box_append_child(LayoutBox* parent, LayoutBox* child);

void layout_context_init(LayoutContext* ctx, float viewport_width, float viewport_height);
void layout_context_free(LayoutContext* ctx);

LayoutBox* layout_compute(const DomNode* dom_root, LayoutContext* ctx);
void layout_compute_box(LayoutBox* box, LayoutContext* ctx, float available_width);
void layout_compute_flex(LayoutBox* flex_box, LayoutContext* ctx);
void layout_compute_position(LayoutBox* box, LayoutContext* ctx);

BoxModel box_model_default(void);
void box_model_compute_size(BoxModel* box, float available_width, float available_height);
float box_model_total_width(const BoxModel* box);
float box_model_total_height(const BoxModel* box);
float box_model_content_x(const BoxModel* box);
float box_model_content_y(const BoxModel* box);

void layout_print_tree(const LayoutBox* root, int indent);

#endif
