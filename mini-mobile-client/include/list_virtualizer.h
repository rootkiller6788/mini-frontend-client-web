#ifndef LIST_VIRTUALIZER_H
#define LIST_VIRTUALIZER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LV_MAX_CELLS        256
#define LV_MAX_TYPES        16
#define LV_RECYCLE_POOL_MAX 32
#define LV_SECTION_MAX      32

typedef enum {
    LV_CELL_HEADER  = 0,
    LV_CELL_ITEM    = 1,
    LV_CELL_FOOTER  = 2,
    LV_CELL_SECTION_HEADER = 3,
    LV_CELL_SECTION_FOOTER = 4,
    LV_CELL_SEPARATOR      = 5
} lv_cell_type_t;

typedef enum {
    LV_SCROLL_IDLE       = 0,
    LV_SCROLL_DRAGGING   = 1,
    LV_SCROLL_DECELERATE = 2,
    LV_SCROLL_ANIMATING  = 3
} lv_scroll_state_t;

typedef struct lv_cell {
    int            id;
    lv_cell_type_t type;
    int            section;
    int            index;
    float          height;
    float          width;
    float          y_offset;
    void          *data;
    bool           recycled;
    bool           visible;
} lv_cell_t;

typedef struct lv_cell_blueprint {
    lv_cell_type_t type;
    float          default_height;
    const char    *reuse_id;
} lv_cell_blueprint_t;

typedef struct lv_section {
    int    start_index;
    int    item_count;
    char   header_title[64];
    char   footer_title[64];
} lv_section_t;

typedef lv_cell_t *(*lv_render_cell_fn)(int index, lv_cell_t *recycled, void *ctx);
typedef float       (*lv_measure_cell_fn)(int index, lv_cell_type_t type, void *ctx);
typedef void        (*lv_on_scroll_fn)(float offset, lv_scroll_state_t state, void *ctx);
typedef void        (*lv_on_refresh_fn)(void *ctx);
typedef void        (*lv_on_end_reached_fn)(void *ctx);
typedef void        (*lv_on_press_fn)(int index, void *ctx);

typedef struct lv_controller {
    int            item_total;
    int            item_estimated;
    float          viewport_height;
    float          viewport_width;
    float          content_height;
    float          scroll_offset;

    lv_cell_t      visible_cells[LV_MAX_CELLS];
    int            visible_count;

    lv_cell_t      recycle_pool[LV_RECYCLE_POOL_MAX];
    int            recycle_count;

    lv_cell_blueprint_t types[LV_MAX_TYPES];
    int             type_count;

    lv_section_t    sections[LV_SECTION_MAX];
    int             section_count;

    lv_render_cell_fn   render_fn;
    lv_measure_cell_fn  measure_fn;
    lv_on_scroll_fn     scroll_fn;
    lv_on_refresh_fn    refresh_fn;
    lv_on_end_reached_fn end_reached_fn;
    lv_on_press_fn      press_fn;
    void               *ctx;

    bool             refreshing;
    bool             loading_more;
    bool             has_more;
    float            refresh_threshold;
    float            end_reached_threshold;

    lv_scroll_state_t scroll_state;
} lv_controller_t;

void   lv_init             (lv_controller_t *ctrl);
void   lv_destroy          (lv_controller_t *ctrl);

void   lv_set_item_count   (lv_controller_t *ctrl, int count);
int    lv_get_item_count   (lv_controller_t *ctrl);

int    lv_register_cell_type(lv_controller_t *ctrl, lv_cell_type_t type,
                             float default_height, const char *reuse_id);

void   lv_set_render_fn    (lv_controller_t *ctrl, lv_render_cell_fn fn, void *ctx);
void   lv_set_measure_fn   (lv_controller_t *ctrl, lv_measure_cell_fn fn, void *ctx);
void   lv_set_scroll_fn    (lv_controller_t *ctrl, lv_on_scroll_fn fn, void *ctx);
void   lv_set_refresh_fn   (lv_controller_t *ctrl, lv_on_refresh_fn fn, void *ctx);
void   lv_set_end_reached_fn(lv_controller_t *ctrl, lv_on_end_reached_fn fn, void *ctx);
void   lv_set_press_fn     (lv_controller_t *ctrl, lv_on_press_fn fn, void *ctx);

int    lv_add_section      (lv_controller_t *ctrl, int start, int count,
                            const char *header, const char *footer);

void   lv_scroll_to_offset (lv_controller_t *ctrl, float offset, bool animated);
void   lv_scroll_to_index  (lv_controller_t *ctrl, int index, bool animated);

void   lv_set_viewport     (lv_controller_t *ctrl, float width, float height);
void   lv_set_scroll_offset(lv_controller_t *ctrl, float offset);

int    lv_get_first_visible(lv_controller_t *ctrl);
int    lv_get_last_visible (lv_controller_t *ctrl);
int    lv_get_visible_count(lv_controller_t *ctrl);

void   lv_begin_refresh    (lv_controller_t *ctrl);
void   lv_end_refresh      (lv_controller_t *ctrl);

void   lv_set_has_more     (lv_controller_t *ctrl, bool has_more);
void   lv_load_more_done   (lv_controller_t *ctrl, int new_items);

void   lv_relayout         (lv_controller_t *ctrl);

void   lv_send_pull_refresh(lv_controller_t *ctrl);
void   lv_send_end_reached (lv_controller_t *ctrl);

lv_cell_t *lv_get_recycled_cell(lv_controller_t *ctrl, lv_cell_type_t type);
void        lv_recycle_cell     (lv_controller_t *ctrl, lv_cell_t *cell);

int    lv_index_for_point  (lv_controller_t *ctrl, float x, float y);

#ifdef __cplusplus
}
#endif

#endif
