#include "render_pipeline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void rp_init(RenderPipeline *rp, double fps) {
    memset(rp, 0, sizeof(*rp));
    rp->target_fps = fps;
    rp->frame_budget_us = (uint64_t)(1e6 / fps);
    rp->vsync_enabled = true;
    rp->compositing_enabled = true;
    rp->gpu_raster_enabled = true;
    rp->raf_callbacks_capacity = 64;
    rp->raf_callbacks = (FrameCallback *)calloc(rp->raf_callbacks_capacity, sizeof(FrameCallback));
    rp->raf_user_data = (void **)calloc(rp->raf_callbacks_capacity, sizeof(void *));
    rp->current_frame.state = FRAME_IDLE;
}

void rp_destroy(RenderPipeline *rp) {
    free(rp->raf_callbacks);
    free(rp->raf_user_data);
    memset(rp, 0, sizeof(*rp));
}

void rp_set_frame_budget_us(RenderPipeline *rp, uint64_t budget_us) {
    rp->frame_budget_us = budget_us;
}

void rp_set_vsync(RenderPipeline *rp, bool enabled) {
    rp->vsync_enabled = enabled;
}

void rp_request_animation_frame(RenderPipeline *rp, FrameCallback cb, void *user_data) {
    if (!cb) return;
    if (rp->raf_callbacks_count >= rp->raf_callbacks_capacity) {
        uint64_t new_cap = rp->raf_callbacks_capacity * 2;
        rp->raf_callbacks = (FrameCallback *)realloc(rp->raf_callbacks, new_cap * sizeof(FrameCallback));
        rp->raf_user_data = (void **)realloc(rp->raf_user_data, new_cap * sizeof(void *));
        rp->raf_callbacks_capacity = new_cap;
    }
    for (uint64_t i = 0; i < rp->raf_callbacks_count; i++) {
        if (rp->raf_callbacks[i] == cb && rp->raf_user_data[i] == user_data) return;
    }
    rp->raf_callbacks[rp->raf_callbacks_count] = cb;
    rp->raf_user_data[rp->raf_callbacks_count] = user_data;
    rp->raf_callbacks_count++;
}

void rp_cancel_animation_frame(RenderPipeline *rp, FrameCallback cb) {
    for (uint64_t i = 0; i < rp->raf_callbacks_count; i++) {
        if (rp->raf_callbacks[i] == cb) {
            uint64_t tail = rp->raf_callbacks_count - 1;
            if (i < tail) {
                rp->raf_callbacks[i] = rp->raf_callbacks[tail];
                rp->raf_user_data[i] = rp->raf_user_data[tail];
            }
            rp->raf_callbacks[tail] = NULL;
            rp->raf_user_data[tail] = NULL;
            rp->raf_callbacks_count--;
            return;
        }
    }
}

void rp_begin_frame(RenderPipeline *rp, uint64_t timestamp_us) {
    FrameInfo *fi = &rp->current_frame;
    fi->frame_id = ++rp->frame_counter;
    fi->vsync_timestamp_us = timestamp_us;
    fi->deadline_us = timestamp_us + rp->frame_budget_us;
    fi->state = FRAME_IN_PROGRESS;
    fi->missed_deadline = false;
    memset(fi->elapsed_us, 0, sizeof(fi->elapsed_us));
    fi->blocking = RENDER_BLOCK_NONE;

    if (rp->render_blocking_css)  fi->blocking |= RENDER_BLOCK_CSS;
    if (rp->render_blocking_js)   fi->blocking |= RENDER_BLOCK_JS_SYNC;

    for (uint64_t i = 0; i < rp->raf_callbacks_count; i++) {
        if (rp->raf_callbacks[i])
            rp->raf_callbacks[i](timestamp_us / 1000, rp->raf_user_data[i]);
    }
    rp->raf_callbacks_count = 0;
}

FrameInfo rp_get_frame_info(const RenderPipeline *rp) {
    return rp->current_frame;
}

void rp_simulate_vsync(RenderPipeline *rp, uint64_t elapsed_us) {
    if (!rp->vsync_enabled) return;
    uint64_t now = rp->current_frame.vsync_timestamp_us + elapsed_us;
    if (now >= rp->current_frame.deadline_us) {
        rp->current_frame.state = FRAME_DROPPED;
        rp->current_frame.missed_deadline = true;
    }
    rp_full_frame(rp, now);
}

static void rp_fire_hook(RenderPipeline *rp, PipelineStage stage) {
    if (rp->hooks[stage]) rp->hooks[stage](stage, rp->hook_user_data[stage]);
}

void rp_perform_style_recalc(RenderPipeline *rp) {
    rp_fire_hook(rp, STYLE_RECALC);
    ComputedStyle *s = rp->tree.styles;
    for (int i = 0; i < rp->tree.style_count; i++) {
        float base_opacity = s[i].opacity > 0 ? s[i].opacity : 1.0f;
        s[i].opacity = base_opacity;
    }
}

void rp_perform_layout(RenderPipeline *rp) {
    rp_fire_hook(rp, LAYOUT_BUILD);
    LayoutBox *boxes = rp->tree.boxes;
    for (int i = 0; i < rp->tree.box_count; i++) {
        boxes[i].scroll_width  = boxes[i].width;
        boxes[i].scroll_height = boxes[i].height;
        if (boxes[i].is_containing_block) {
            boxes[i].is_positioned = true;
        }
    }
}

void rp_perform_paint(RenderPipeline *rp) {
    rp_fire_hook(rp, PAINT_GENERATE);
    for (int i = 0; i < rp->tree.layer_count; i++) {
        PaintLayer *pl = &rp->tree.layers[i];
        if (pl->needs_repaint) {
            pl->needs_repaint = false;
        }
    }
}

void rp_perform_composite(RenderPipeline *rp) {
    rp_fire_hook(rp, COMPOSITE_MERGE);
    for (int i = 0; i < rp->tree.layer_count; i++) {
        PaintLayer *pl = &rp->tree.layers[i];
        if (pl->is_composited && pl->opacity < 1.0f) {
            pl->needs_repaint = true;
        }
    }
}

void rp_full_frame(RenderPipeline *rp, uint64_t timestamp_us) {
    rp_begin_frame(rp, timestamp_us);
    uint64_t t0 = timestamp_us;

    rp_perform_style_recalc(rp);
    uint64_t t1 = timestamp_us + 200;
    rp->current_frame.elapsed_us[STYLE_RECALC] = (t1 > t0) ? (t1 - t0) : 0;

    rp_perform_layout(rp);
    uint64_t t2 = t1 + 300;
    rp->current_frame.elapsed_us[LAYOUT_BUILD] = (t2 > t1) ? (t2 - t1) : 0;

    rp_perform_paint(rp);
    uint64_t t3 = t2 + 500;
    rp->current_frame.elapsed_us[PAINT_GENERATE] = (t3 > t2) ? (t3 - t2) : 0;

    rp_perform_composite(rp);
    uint64_t t4 = t3 + 200;
    rp->current_frame.elapsed_us[COMPOSITE_MERGE] = (t4 > t3) ? (t4 - t3) : 0;

    uint64_t total = t4 - t0;
    if (total > rp->frame_budget_us) {
        rp->current_frame.missed_deadline = true;
        rp->current_frame.state = FRAME_DROPPED;
    } else {
        rp->current_frame.state = FRAME_COMMITTED;
    }
}

void rp_register_hook(RenderPipeline *rp, PipelineStage stage, PipelineHook hook, void *user_data) {
    if (stage < 0 || stage > 3) return;
    rp->hooks[stage] = hook;
    rp->hook_user_data[stage] = user_data;
}

void rp_disable_stage(RenderPipeline *rp, PipelineStage stage) {
    rp->hooks[stage] = NULL;
}

void rp_enable_stage(RenderPipeline *rp, PipelineStage stage) {
    (void)rp; (void)stage;
}

void rp_add_render_blocking(RenderPipeline *rp, RenderBlocking block) {
    if (block & RENDER_BLOCK_CSS)  rp->render_blocking_css = true;
    if (block & RENDER_BLOCK_JS_SYNC) rp->render_blocking_js = true;
}

void rp_remove_render_blocking(RenderPipeline *rp, RenderBlocking block) {
    if (block & RENDER_BLOCK_CSS)  rp->render_blocking_css = false;
    if (block & RENDER_BLOCK_JS_SYNC) rp->render_blocking_js = false;
}

const char *rp_stage_name(PipelineStage stage) {
    static const char *names[] = { "StyleRecalc", "Layout", "Paint", "Composite" };
    return (stage < 4) ? names[stage] : "Unknown";
}

double rp_frame_duration_ms(const FrameInfo *fi) {
    double total = 0;
    for (int i = 0; i < 4; i++) total += (double)fi->elapsed_us[i];
    return total / 1000.0;
}

double rp_get_utilization(const RenderPipeline *rp) {
    if (rp->frame_budget_us == 0) return 0;
    return rp_frame_duration_ms(&rp->current_frame) * 1000.0 / (double)rp->frame_budget_us;
}
