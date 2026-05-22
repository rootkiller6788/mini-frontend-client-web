#ifndef YOGA_LAYOUT_H
#define YOGA_LAYOUT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define YG_MAX_CHILDREN  64

typedef enum {
    YG_DIRECTION_INHERIT = 0,
    YG_DIRECTION_LTR     = 1,
    YG_DIRECTION_RTL     = 2
} yg_direction_t;

typedef enum {
    YG_FLEX_DIRECTION_COLUMN        = 0,
    YG_FLEX_DIRECTION_COLUMN_REVERSE= 1,
    YG_FLEX_DIRECTION_ROW           = 2,
    YG_FLEX_DIRECTION_ROW_REVERSE   = 3
} yg_flex_direction_t;

typedef enum {
    YG_JUSTIFY_FLEX_START    = 0,
    YG_JUSTIFY_CENTER        = 1,
    YG_JUSTIFY_FLEX_END      = 2,
    YG_JUSTIFY_SPACE_BETWEEN = 3,
    YG_JUSTIFY_SPACE_AROUND  = 4,
    YG_JUSTIFY_SPACE_EVENLY  = 5
} yg_justify_t;

typedef enum {
    YG_ALIGN_AUTO         = 0,
    YG_ALIGN_FLEX_START   = 1,
    YG_ALIGN_CENTER       = 2,
    YG_ALIGN_FLEX_END     = 3,
    YG_ALIGN_STRETCH      = 4,
    YG_ALIGN_BASELINE     = 5,
    YG_ALIGN_SPACE_BETWEEN= 6,
    YG_ALIGN_SPACE_AROUND = 7
} yg_align_t;

typedef enum {
    YG_POSITION_RELATIVE = 0,
    YG_POSITION_ABSOLUTE = 1
} yg_position_t;

typedef enum {
    YG_UNIT_UNDEFINED = 0,
    YG_UNIT_POINT     = 1,
    YG_UNIT_PERCENT   = 2,
    YG_UNIT_AUTO      = 3
} yg_unit_t;

typedef enum {
    YG_DISPLAY_FLEX = 0,
    YG_DISPLAY_NONE = 1
} yg_display_t;

typedef enum {
    YG_OVERFLOW_VISIBLE = 0,
    YG_OVERFLOW_HIDDEN  = 1,
    YG_OVERFLOW_SCROLL  = 2
} yg_overflow_t;

typedef enum {
    YG_EDGE_LEFT    = 0,
    YG_EDGE_TOP     = 1,
    YG_EDGE_RIGHT   = 2,
    YG_EDGE_BOTTOM  = 3,
    YG_EDGE_START   = 4,
    YG_EDGE_END     = 5,
    YG_EDGE_ALL     = 6
} yg_edge_t;

typedef struct yg_value {
    float     value;
    yg_unit_t unit;
} yg_value_t;

typedef struct yg_size {
    float width;
    float height;
} yg_size_t;

typedef struct yg_rect {
    float x, y;
    float width, height;
} yg_rect_t;

typedef struct yg_measure_result {
    float width;
    float height;
    bool  measured;
} yg_measure_result_t;

typedef yg_measure_result_t (*yg_measure_fn)(void *ctx, float width, yg_unit_t width_mode,
                                              float height, yg_unit_t height_mode);

typedef struct yg_node {
    int            id;
    yg_direction_t  direction;
    yg_flex_direction_t flex_direction;
    yg_justify_t   justify_content;
    yg_align_t     align_items;
    yg_align_t     align_content;

    float          flex_grow;
    float          flex_shrink;
    yg_value_t     flex_basis;
    yg_align_t     align_self;

    yg_value_t     width;
    yg_value_t     height;
    yg_value_t     min_width;
    yg_value_t     min_height;
    yg_value_t     max_width;
    yg_value_t     max_height;

    yg_value_t     margin[4];
    yg_value_t     padding[4];
    yg_value_t     border[4];

    yg_value_t     position_val[4];
    yg_position_t  position_type;

    yg_display_t   display;
    yg_overflow_t  overflow;

    float          aspect_ratio;

    yg_rect_t      layout;

    struct yg_node *parent;
    struct yg_node *children[YG_MAX_CHILDREN];
    int            child_count;

    yg_measure_fn  measure;
    void          *measure_ctx;

    void          *user_data;
} yg_node_t;

yg_node_t *yg_node_create     (void);
void       yg_node_destroy    (yg_node_t *node);

void       yg_node_add_child  (yg_node_t *parent, yg_node_t *child);
void       yg_node_remove_child(yg_node_t *parent, yg_node_t *child);
int        yg_node_child_count(yg_node_t *node);
yg_node_t *yg_node_get_child  (yg_node_t *node, int index);

void       yg_node_set_flex_direction   (yg_node_t *node, yg_flex_direction_t val);
void       yg_node_set_justify_content  (yg_node_t *node, yg_justify_t val);
void       yg_node_set_align_items      (yg_node_t *node, yg_align_t val);
void       yg_node_set_align_content    (yg_node_t *node, yg_align_t val);
void       yg_node_set_align_self       (yg_node_t *node, yg_align_t val);

void       yg_node_set_flex_grow        (yg_node_t *node, float val);
void       yg_node_set_flex_shrink      (yg_node_t *node, float val);
void       yg_node_set_flex_basis       (yg_node_t *node, float val, yg_unit_t unit);

void       yg_node_set_width            (yg_node_t *node, float val, yg_unit_t unit);
void       yg_node_set_height           (yg_node_t *node, float val, yg_unit_t unit);
void       yg_node_set_min_width        (yg_node_t *node, float val, yg_unit_t unit);
void       yg_node_set_min_height       (yg_node_t *node, float val, yg_unit_t unit);
void       yg_node_set_max_width        (yg_node_t *node, float val, yg_unit_t unit);
void       yg_node_set_max_height       (yg_node_t *node, float val, yg_unit_t unit);

void       yg_node_set_margin           (yg_node_t *node, yg_edge_t edge, float val, yg_unit_t unit);
void       yg_node_set_padding          (yg_node_t *node, yg_edge_t edge, float val, yg_unit_t unit);
void       yg_node_set_border           (yg_node_t *node, yg_edge_t edge, float val);

void       yg_node_set_position         (yg_node_t *node, yg_edge_t edge, float val, yg_unit_t unit);
void       yg_node_set_position_type    (yg_node_t *node, yg_position_t val);

void       yg_node_set_display          (yg_node_t *node, yg_display_t val);
void       yg_node_set_overflow         (yg_node_t *node, yg_overflow_t val);
void       yg_node_set_aspect_ratio     (yg_node_t *node, float val);

void       yg_node_set_measure_func     (yg_node_t *node, yg_measure_fn fn, void *ctx);
void       yg_node_set_user_data        (yg_node_t *node, void *data);
void      *yg_node_get_user_data        (yg_node_t *node);

void       yg_node_calculate_layout     (yg_node_t *node, float available_width,
                                         float available_height, yg_direction_t direction);
yg_rect_t  yg_node_get_layout           (yg_node_t *node);

yg_value_t yg_value_percent             (float val);
yg_value_t yg_value_point               (float val);
yg_value_t yg_value_auto                (void);
yg_value_t yg_value_undefined           (void);

const char *yg_node_to_json             (yg_node_t *node, char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
