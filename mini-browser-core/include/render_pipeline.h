#ifndef RENDER_PIPELINE_H
#define RENDER_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef enum {
    FRAME_IDLE,
    FRAME_PENDING,
    FRAME_IN_PROGRESS,
    FRAME_COMMITTED,
    FRAME_DROPPED
} FrameState;

typedef enum {
    STYLE_RECALC       = 0,
    LAYOUT_BUILD       = 1,
    PAINT_GENERATE     = 2,
    COMPOSITE_MERGE    = 3
} PipelineStage;

typedef enum {
    RENDER_BLOCK_NONE        = 0,
    RENDER_BLOCK_CSS         = 1 << 0,
    RENDER_BLOCK_JS_SYNC     = 1 << 1,
    RENDER_BLOCK_JS_ASYNC    = 1 << 2,
    RENDER_BLOCK_FONT        = 1 << 3,
    RENDER_BLOCK_IMPORT      = 1 << 4
} RenderBlocking;

typedef struct {
    uint64_t id;
    char     tag[32];
    char     class_list[256];
    char     id_attr[64];
} DomNode;

typedef struct {
    uint64_t node_id;
    float    x, y, width, height;
    uint32_t color;
    float    font_size;
    float    opacity;
    float    z_index;
    bool     visible;
    bool     has_transform;
    float    transform_matrix[6];
} ComputedStyle;

typedef struct {
    uint64_t     node_id;
    float        x, y, width, height;
    float        scroll_width, scroll_height;
    int32_t      child_count;
    uint64_t     children[128];
    bool         is_containing_block;
    bool         establishes_stacking_context;
    bool         is_positioned;
} LayoutBox;

typedef struct {
    uint64_t     layer_id;
    uint64_t     node_id;
    float        bounds[4];
    uint8_t      r, g, b, a;
    float        opacity;
    float        transform[6];
    int32_t      child_layer_count;
    uint64_t     child_layers[32];
    bool         needs_repaint;
    bool         is_composited;
    char         compositing_reason[64];
} PaintLayer;

typedef struct {
    uint64_t    frame_id;
    uint64_t    vsync_timestamp_us;
    uint64_t    deadline_us;
    FrameState  state;
    int64_t     elapsed_us[4];
    bool        missed_deadline;
    RenderBlocking blocking;
} FrameInfo;

typedef struct {
    ComputedStyle styles[256];
    int32_t       style_count;
    LayoutBox     boxes[256];
    int32_t       box_count;
    PaintLayer    layers[64];
    int32_t       layer_count;
} RenderTree;

typedef void (*FrameCallback)(uint64_t timestamp_ms, void *user_data);
typedef void (*PipelineHook)(PipelineStage stage, void *user_data);

typedef struct {
    RenderTree     tree;
    FrameInfo      current_frame;
    uint64_t       frame_counter;
    uint64_t       frame_budget_us;
    double         target_fps;
    bool           vsync_enabled;
    bool           compositing_enabled;
    bool           gpu_raster_enabled;
    bool           render_blocking_css;
    bool           render_blocking_js;
    uint64_t       raf_callbacks_capacity;
    uint64_t       raf_callbacks_count;
    FrameCallback *raf_callbacks;
    void         **raf_user_data;
    PipelineHook   hooks[4];
    void          *hook_user_data[4];
} RenderPipeline;

void rp_init(RenderPipeline *rp, double fps);
void rp_destroy(RenderPipeline *rp);
void rp_set_frame_budget_us(RenderPipeline *rp, uint64_t budget_us);
void rp_set_vsync(RenderPipeline *rp, bool enabled);
void rp_request_animation_frame(RenderPipeline *rp, FrameCallback cb, void *user_data);
void rp_cancel_animation_frame(RenderPipeline *rp, FrameCallback cb);
void rp_begin_frame(RenderPipeline *rp, uint64_t timestamp_us);
FrameInfo rp_get_frame_info(const RenderPipeline *rp);
void rp_simulate_vsync(RenderPipeline *rp, uint64_t elapsed_us);
void rp_perform_style_recalc(RenderPipeline *rp);
void rp_perform_layout(RenderPipeline *rp);
void rp_perform_paint(RenderPipeline *rp);
void rp_perform_composite(RenderPipeline *rp);
void rp_full_frame(RenderPipeline *rp, uint64_t timestamp_us);
void rp_register_hook(RenderPipeline *rp, PipelineStage stage, PipelineHook hook, void *user_data);
void rp_disable_stage(RenderPipeline *rp, PipelineStage stage);
void rp_enable_stage(RenderPipeline *rp, PipelineStage stage);
void rp_add_render_blocking(RenderPipeline *rp, RenderBlocking block);
void rp_remove_render_blocking(RenderPipeline *rp, RenderBlocking block);
const char *rp_stage_name(PipelineStage stage);
double rp_frame_duration_ms(const FrameInfo *fi);
double rp_get_utilization(const RenderPipeline *rp);

#endif
