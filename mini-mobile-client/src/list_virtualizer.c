#include "list_virtualizer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

void lv_init(lv_controller_t *ctrl)
{
    if (!ctrl) return;
    memset(ctrl, 0, sizeof(*ctrl));

    ctrl->item_total       = 0;
    ctrl->item_estimated   = 50;
    ctrl->viewport_height  = 600.0f;
    ctrl->viewport_width   = 375.0f;
    ctrl->content_height   = 0.0f;
    ctrl->scroll_offset    = 0.0f;
    ctrl->visible_count    = 0;
    ctrl->recycle_count    = 0;
    ctrl->type_count       = 0;
    ctrl->section_count    = 0;
    ctrl->refreshing       = false;
    ctrl->loading_more     = false;
    ctrl->has_more         = true;
    ctrl->refresh_threshold= 80.0f;
    ctrl->end_reached_threshold = 200.0f;
    ctrl->scroll_state     = LV_SCROLL_IDLE;

    lv_register_cell_type(ctrl, LV_CELL_ITEM, 50.0f, "default_cell");
}

void lv_destroy(lv_controller_t *ctrl)
{
    if (!ctrl) return;
    memset(ctrl, 0, sizeof(*ctrl));
}

void lv_set_item_count(lv_controller_t *ctrl, int count)
{
    if (!ctrl) return;
    ctrl->item_total = count;

    if (ctrl->measure_fn) {
        float total = 0.0f;
        for (int i = 0; i < count; i++) {
            total += ctrl->measure_fn(i, LV_CELL_ITEM, ctrl->ctx);
        }
        ctrl->content_height = total;
    } else {
        ctrl->content_height = (float)count * (float)ctrl->item_estimated;
    }
}

int lv_get_item_count(lv_controller_t *ctrl)
{
    return ctrl ? ctrl->item_total : 0;
}

int lv_register_cell_type(lv_controller_t *ctrl, lv_cell_type_t type,
                           float default_height, const char *reuse_id)
{
    if (!ctrl || ctrl->type_count >= LV_MAX_TYPES) return -1;

    lv_cell_blueprint_t *bp = &ctrl->types[ctrl->type_count++];
    bp->type           = type;
    bp->default_height = default_height;
    if (reuse_id) {
        strncpy((char *)bp->reuse_id, reuse_id, sizeof(bp->reuse_id) - 1);
    }
    return ctrl->type_count - 1;
}

void lv_set_render_fn(lv_controller_t *ctrl, lv_render_cell_fn fn, void *ctx)
{ if (ctrl) { ctrl->render_fn = fn; ctrl->ctx = ctx; } }

void lv_set_measure_fn(lv_controller_t *ctrl, lv_measure_cell_fn fn, void *ctx)
{ if (ctrl) { ctrl->measure_fn = fn; ctrl->ctx = ctx; } }

void lv_set_scroll_fn(lv_controller_t *ctrl, lv_on_scroll_fn fn, void *ctx)
{ if (ctrl) { ctrl->scroll_fn = fn; ctrl->ctx = ctx; } }

void lv_set_refresh_fn(lv_controller_t *ctrl, lv_on_refresh_fn fn, void *ctx)
{ if (ctrl) { ctrl->refresh_fn = fn; ctrl->ctx = ctx; } }

void lv_set_end_reached_fn(lv_controller_t *ctrl, lv_on_end_reached_fn fn, void *ctx)
{ if (ctrl) { ctrl->end_reached_fn = fn; ctrl->ctx = ctx; } }

void lv_set_press_fn(lv_controller_t *ctrl, lv_on_press_fn fn, void *ctx)
{ if (ctrl) { ctrl->press_fn = fn; ctrl->ctx = ctx; } }

void lv_set_viewport(lv_controller_t *ctrl, float width, float height)
{
    if (!ctrl) return;
    ctrl->viewport_width  = width;
    ctrl->viewport_height = height;
    lv_relayout(ctrl);
}

void lv_set_scroll_offset(lv_controller_t *ctrl, float offset)
{
    if (!ctrl) return;
    if (offset < 0.0f) offset = 0.0f;

    float max_offset = ctrl->content_height - ctrl->viewport_height;
    if (max_offset < 0.0f) max_offset = 0.0f;
    if (offset > max_offset) offset = max_offset;

    ctrl->scroll_offset = offset;

    if (ctrl->scroll_fn) {
        ctrl->scroll_fn(offset, ctrl->scroll_state, ctrl->ctx);
    }

    if (ctrl->scroll_offset <= -ctrl->refresh_threshold && !ctrl->refreshing) {
        lv_send_pull_refresh(ctrl);
    }

    float remaining = ctrl->content_height - ctrl->viewport_height - ctrl->scroll_offset;
    if (remaining <= ctrl->end_reached_threshold && ctrl->has_more && !ctrl->loading_more) {
        lv_send_end_reached(ctrl);
    }
}

void lv_scroll_to_offset(lv_controller_t *ctrl, float offset, bool animated)
{
    if (!ctrl) return;
    (void)animated;
    lv_set_scroll_offset(ctrl, offset);
}

void lv_scroll_to_index(lv_controller_t *ctrl, int index, bool animated)
{
    if (!ctrl || index < 0 || index >= ctrl->item_total) return;

    float offset = 0.0f;
    if (ctrl->measure_fn) {
        for (int i = 0; i < index; i++) {
            offset += ctrl->measure_fn(i, LV_CELL_ITEM, ctrl->ctx);
        }
    } else {
        offset = (float)index * (float)ctrl->item_estimated;
    }

    (void)animated;
    lv_set_scroll_offset(ctrl, offset);
}

static void compute_visible_range(lv_controller_t *ctrl, int *first_out, int *last_out)
{
    if (!ctrl) { *first_out = 0; *last_out = 0; return; }

    float top    = ctrl->scroll_offset;
    float bottom = top + ctrl->viewport_height;

    int first = 0, last = 0;
    float accum = 0.0f;
    bool found_first = false;

    for (int i = 0; i < ctrl->item_total; i++) {
        float h;
        if (ctrl->measure_fn) {
            h = ctrl->measure_fn(i, LV_CELL_ITEM, ctrl->ctx);
        } else {
            h = (float)ctrl->item_estimated;
        }

        if (!found_first && accum + h > top) {
            first = i;
            found_first = true;
        }

        if (found_first && accum > bottom) {
            last = i;
            break;
        }

        accum += h;
        last = i + 1;
    }

    first_out[0] = first;
    last_out[0]  = last > ctrl->item_total ? ctrl->item_total : last;
}

void lv_relayout(lv_controller_t *ctrl)
{
    if (!ctrl || ctrl->item_total == 0) return;

    int first, last;
    compute_visible_range(ctrl, &first, &last);

    lv_cell_t existing[LV_MAX_CELLS];
    int existing_count = ctrl->visible_count;
    memcpy(existing, ctrl->visible_cells, existing_count * sizeof(lv_cell_t));

    for (int i = 0; i < existing_count; i++) {
        if (existing[i].index < first || existing[i].index >= last) {
            lv_recycle_cell(ctrl, &existing[i]);
        }
    }

    ctrl->visible_count = 0;

    float accum = 0.0f;
    for (int i = 0; i < last; i++) {
        float h;
        if (ctrl->measure_fn) {
            h = ctrl->measure_fn(i, LV_CELL_ITEM, ctrl->ctx);
        } else {
            h = (float)ctrl->item_estimated;
        }

        if (i >= first) {
            lv_cell_t *cell = lv_get_recycled_cell(ctrl, LV_CELL_ITEM);
            if (!cell && ctrl->visible_count < LV_MAX_CELLS) {
                cell = &ctrl->visible_cells[ctrl->visible_count];
            }
            if (cell && ctrl->visible_count < LV_MAX_CELLS) {
                cell->id       = i;
                cell->type     = LV_CELL_ITEM;
                cell->index    = i;
                cell->height   = h;
                cell->width    = ctrl->viewport_width;
                cell->y_offset = accum;
                cell->visible  = true;
                cell->recycled = false;

                if (ctrl->render_fn) {
                    lv_cell_t *reused = NULL;
                    for (int j = 0; j < existing_count; j++) {
                        if (existing[j].index == i && !existing[j].recycled) {
                            reused = &existing[j];
                            break;
                        }
                    }
                    ctrl->render_fn(i, reused, ctrl->ctx);
                    memcpy(cell, reused ? reused : cell, sizeof(lv_cell_t));
                }

                ctrl->visible_count++;
            }
        }

        accum += h;
    }

    ctrl->content_height = accum;
}

int lv_get_first_visible(lv_controller_t *ctrl)
{
    if (!ctrl || ctrl->visible_count == 0) return 0;
    return ctrl->visible_cells[0].index;
}

int lv_get_last_visible(lv_controller_t *ctrl)
{
    if (!ctrl || ctrl->visible_count == 0) return 0;
    return ctrl->visible_cells[ctrl->visible_count - 1].index;
}

int lv_get_visible_count(lv_controller_t *ctrl)
{
    return ctrl ? ctrl->visible_count : 0;
}

void lv_begin_refresh(lv_controller_t *ctrl)
{
    if (!ctrl) return;
    ctrl->refreshing = true;
    if (ctrl->refresh_fn) ctrl->refresh_fn(ctrl->ctx);
}

void lv_end_refresh(lv_controller_t *ctrl)
{
    if (!ctrl) return;
    ctrl->refreshing = false;
    ctrl->scroll_offset = 0.0f;
}

void lv_set_has_more(lv_controller_t *ctrl, bool has_more)
{
    if (ctrl) ctrl->has_more = has_more;
}

void lv_load_more_done(lv_controller_t *ctrl, int new_items)
{
    if (!ctrl) return;
    ctrl->loading_more = false;
    ctrl->item_total += new_items;
    lv_set_item_count(ctrl, ctrl->item_total);
    lv_relayout(ctrl);
}

void lv_send_pull_refresh(lv_controller_t *ctrl)
{
    if (ctrl && ctrl->refresh_fn) {
        ctrl->refresh_fn(ctrl->ctx);
    }
}

void lv_send_end_reached(lv_controller_t *ctrl)
{
    if (!ctrl || !ctrl->has_more || ctrl->loading_more) return;
    ctrl->loading_more = true;
    if (ctrl->end_reached_fn) {
        ctrl->end_reached_fn(ctrl->ctx);
    }
}

lv_cell_t *lv_get_recycled_cell(lv_controller_t *ctrl, lv_cell_type_t type)
{
    if (!ctrl) return NULL;

    for (int i = 0; i < ctrl->recycle_count; i++) {
        if (ctrl->recycle_pool[i].type == type) {
            lv_cell_t *cell = &ctrl->recycle_pool[i];
            cell->recycled = false;
            memmove(&ctrl->recycle_pool[i], &ctrl->recycle_pool[i + 1],
                    (ctrl->recycle_count - i - 1) * sizeof(lv_cell_t));
            ctrl->recycle_count--;
            return cell;
        }
    }

    return NULL;
}

void lv_recycle_cell(lv_controller_t *ctrl, lv_cell_t *cell)
{
    if (!ctrl || !cell || ctrl->recycle_count >= LV_RECYCLE_POOL_MAX) return;

    cell->recycled = true;
    cell->visible  = false;
    memcpy(&ctrl->recycle_pool[ctrl->recycle_count++], cell, sizeof(lv_cell_t));
}

int lv_add_section(lv_controller_t *ctrl, int start, int count,
                    const char *header, const char *footer)
{
    if (!ctrl || ctrl->section_count >= LV_SECTION_MAX) return -1;

    lv_section_t *sec = &ctrl->sections[ctrl->section_count++];
    sec->start_index = start;
    sec->item_count  = count;
    if (header) strncpy(sec->header_title, header, sizeof(sec->header_title) - 1);
    if (footer) strncpy(sec->footer_title, footer, sizeof(sec->footer_title) - 1);

    return ctrl->section_count - 1;
}

int lv_index_for_point(lv_controller_t *ctrl, float x, float y)
{
    if (!ctrl) return -1;
    (void)x;

    float accum = 0.0f;
    for (int i = 0; i < ctrl->item_total; i++) {
        float h;
        if (ctrl->measure_fn) {
            h = ctrl->measure_fn(i, LV_CELL_ITEM, ctrl->ctx);
        } else {
            h = (float)ctrl->item_estimated;
        }

        if (y >= accum && y < accum + h) return i;
        accum += h;
    }

    return -1;
}
