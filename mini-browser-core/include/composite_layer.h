#ifndef COMPOSITE_LAYER_H
#define COMPOSITE_LAYER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_LAYERS           128
#define MAX_TILES_PER_LAYER  64
#define TILE_SIZE_PX         256
#define MAX_COMPOSITING_REASONS 8

typedef enum {
    LAYER_ROOT             = 0,
    LAYER_PAINT            = 1,
    LAYER_GRAPHICS         = 2,
    LAYER_VIDEO            = 3,
    LAYER_IFRAME           = 4,
    LAYER_WEBGL            = 5,
    LAYER_SCROLL           = 6,
    LAYER_OVERLAY          = 7,
    LAYER_TRANSFORM        = 8,
    LAYER_OPACITY          = 9,
    LAYER_FILTER           = 10,
    LAYER_MASK             = 11
} LayerType;

typedef enum {
    COMP_REASON_TRANSFORM_3D      = 1 << 0,
    COMP_REASON_TRANSFORM_ANIM    = 1 << 1,
    COMP_REASON_OPACITY_ANIM      = 1 << 2,
    COMP_REASON_FILTER_ANIM       = 1 << 3,
    COMP_REASON_VIDEO             = 1 << 4,
    COMP_REASON_IFRAME            = 1 << 5,
    COMP_REASON_WEBGL             = 1 << 6,
    COMP_REASON_SCROLL            = 1 << 7,
    COMP_REASON_WILL_CHANGE       = 1 << 8,
    COMP_REASON_OVERLAP           = 1 << 9,
    COMP_REASON_STACKING_CONTEXT  = 1 << 10,
    COMP_REASON_FIXED_POSITION    = 1 << 11,
    COMP_REASON_ANIMATION         = 1 << 12
} CompositingReason;

typedef enum {
    TILE_EMPTY        = 0,
    TILE_PENDING      = 1,
    TILE_RASTERIZING  = 2,
    TILE_READY        = 3,
    TILE_EVICTED      = 4
} TileState;

typedef struct {
    uint32_t  tile_id;
    uint32_t  col, row;
    uint32_t  x, y;
    uint32_t  width, height;
    TileState state;
    uint8_t  *pixels;
    uint32_t  pixel_size;
    bool      is_dirty;
    bool      is_visible;
    uint8_t   priority;
} LayerTile;

typedef struct {
    uint64_t           layer_id;
    LayerType          type;
    int32_t            parent_id;
    int32_t            child_ids[32];
    int32_t            child_count;
    float              bounds[4];
    float              position[2];
    float              anchor[2];
    float              transform[16];
    float              opacity;
    float              z_index;
    bool               visible;
    bool               should_flatten;
    bool               is_composited;
    uint32_t           compositing_reasons;
    bool               has_will_change;
    bool               is_scrollable;
    bool               is_fixed_position;
    bool               clips_children;

    LayerTile          tiles[MAX_TILES_PER_LAYER];
    uint32_t           tile_count;
    uint32_t           tile_cols;
    uint32_t           tile_rows;

    uint8_t           *backing_store;
    uint64_t           backing_size;
    bool               backing_dirty;

    uint64_t           last_raster_ms;
    uint64_t           last_commit_ms;
    uint32_t           raster_count;
    uint64_t           memory_usage;
} Layer;

typedef struct {
    Layer              layers[MAX_LAYERS];
    int32_t            layer_count;
    int32_t            root_layer_idx;
    uint64_t           layer_id_counter;
    uint64_t           total_memory;

    bool               impl_side_painting;
    bool               zero_copy_enabled;
    bool               gpu_rasterization;

    uint64_t           frame_counter;
    uint64_t           tiles_rasterized;
    uint64_t           tiles_skipped;
    uint64_t           total_raster_ms;

    int32_t            viewport_width;
    int32_t            viewport_height;
    float              device_scale_factor;
} Compositor;

void comp_init(Compositor *c, int32_t vp_w, int32_t vp_h, float dsf);
void comp_destroy(Compositor *c);

int32_t comp_create_layer(Compositor *c, LayerType type, int32_t parent_id);
Layer *comp_get_layer(Compositor *c, int32_t idx);
void comp_remove_layer(Compositor *c, int32_t idx);
void comp_set_layer_transform(Layer *l, const float matrix[16]);
void comp_set_layer_opacity(Layer *l, float opacity);
void comp_set_layer_bounds(Layer *l, float x, float y, float w, float h);
void comp_set_layer_position(Layer *l, float x, float y);
void comp_add_compositing_reason(Layer *l, CompositingReason reason);
bool comp_has_reason(const Layer *l, CompositingReason reason);
void comp_evaluate_compositing(Layer *l);

void comp_set_viewport(Compositor *c, int32_t w, int32_t h, float dsf);
void comp_tile_layer(Compositor *c, int32_t layer_idx);
void comp_rasterize_tile(Layer *l, uint32_t tile_id);
void comp_rasterize_dirty(Compositor *c);
void comp_commit_frame(Compositor *c);

void comp_calculate_visible_tiles(Compositor *c, float scroll_x, float scroll_y);
void comp_update_tile_priorities(Compositor *c);
void comp_evict_tiles(Compositor *c, uint64_t max_bytes);

void comp_set_impl_side_painting(Compositor *c, bool enabled);
void comp_set_gpu_raster(Compositor *c, bool enabled);
void comp_set_zero_copy(Compositor *c, bool enabled);

bool comp_is_composited_anim(const Layer *l);
const char *comp_layer_type_name(LayerType type);
const char *comp_reason_name(CompositingReason reason);
uint64_t comp_get_total_memory(const Compositor *c);
void comp_dump_layer_tree(const Compositor *c, int32_t idx, int32_t depth);

#endif
