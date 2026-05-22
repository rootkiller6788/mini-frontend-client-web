#include "composite_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void comp_init(Compositor *c, int32_t vp_w, int32_t vp_h, float dsf) {
    memset(c, 0, sizeof(*c));
    c->viewport_width = vp_w;
    c->viewport_height = vp_h;
    c->device_scale_factor = dsf > 0 ? dsf : 1.0f;
    c->impl_side_painting = true;
    c->gpu_rasterization = true;
    c->zero_copy_enabled = false;
    c->root_layer_idx = comp_create_layer(c, LAYER_ROOT, -1);
    c->layers[c->root_layer_idx].visible = true;
    Layer *root = &c->layers[c->root_layer_idx];
    comp_set_layer_bounds(root, 0, 0, (float)vp_w, (float)vp_h);
    comp_set_layer_position(root, 0, 0);
}

void comp_destroy(Compositor *c) {
    for (int32_t i = 0; i < c->layer_count; i++) {
        free(c->layers[i].backing_store);
        for (uint32_t t = 0; t < c->layers[i].tile_count; t++)
            free(c->layers[i].tiles[t].pixels);
    }
    memset(c, 0, sizeof(*c));
}

int32_t comp_create_layer(Compositor *c, LayerType type, int32_t parent_id) {
    if (c->layer_count >= MAX_LAYERS) return -1;
    int32_t idx = c->layer_count++;
    Layer *l = &c->layers[idx];
    memset(l, 0, sizeof(*l));
    l->layer_id = ++c->layer_id_counter;
    l->type = type;
    l->parent_id = parent_id;
    l->opacity = 1.0f;
    l->visible = true;
    l->should_flatten = false;
    l->is_composited = false;
    for (int i = 0; i < 16; i++) l->transform[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    if (parent_id >= 0 && parent_id < c->layer_count) {
        Layer *p = &c->layers[parent_id];
        if (p->child_count < 32) p->child_ids[p->child_count++] = idx;
    }
    return idx;
}

Layer *comp_get_layer(Compositor *c, int32_t idx) {
    if (idx < 0 || idx >= c->layer_count) return NULL;
    return &c->layers[idx];
}

void comp_remove_layer(Compositor *c, int32_t idx) {
    if (idx < 0 || idx >= c->layer_count) return;
    Layer *l = &c->layers[idx];
    free(l->backing_store); l->backing_store = NULL;
    for (uint32_t t = 0; t < l->tile_count; t++) free(l->tiles[t].pixels);
    memset(l, 0, sizeof(*l));
    l->visible = false;
}

void comp_set_layer_transform(Layer *l, const float matrix[16]) {
    memcpy(l->transform, matrix, sizeof(float) * 16);
}

void comp_set_layer_opacity(Layer *l, float opacity) {
    l->opacity = opacity < 0 ? 0 : (opacity > 1 ? 1 : opacity);
}

void comp_set_layer_bounds(Layer *l, float x, float y, float w, float h) {
    l->bounds[0] = x; l->bounds[1] = y;
    l->bounds[2] = w; l->bounds[3] = h;
}

void comp_set_layer_position(Layer *l, float x, float y) {
    l->position[0] = x; l->position[1] = y;
}

void comp_add_compositing_reason(Layer *l, CompositingReason reason) {
    l->compositing_reasons |= (uint32_t)reason;
    l->is_composited = true;
}

bool comp_has_reason(const Layer *l, CompositingReason reason) {
    return (l->compositing_reasons & (uint32_t)reason) != 0;
}

void comp_evaluate_compositing(Layer *l) {
    if (l->compositing_reasons != 0) { l->is_composited = true; return; }
    bool has_3d = l->transform[2] != 0 || l->transform[6] != 0 || l->transform[10] != 1.0f;
    if (has_3d) { comp_add_compositing_reason((Layer *)l, COMP_REASON_TRANSFORM_3D); return; }
    if (l->opacity < 1.0f) { comp_add_compositing_reason((Layer *)l, COMP_REASON_OPACITY_ANIM); return; }
}

void comp_set_viewport(Compositor *c, int32_t w, int32_t h, float dsf) {
    c->viewport_width = w; c->viewport_height = h;
    c->device_scale_factor = dsf > 0 ? dsf : 1.0f;
}

void comp_tile_layer(Compositor *c, int32_t layer_idx) {
    Layer *l = comp_get_layer(c, layer_idx);
    if (!l || !l->visible) return;
    float bw = l->bounds[2], bh = l->bounds[3];
    if (bw <= 0 || bh <= 0) return;
    l->tile_cols = (uint32_t)ceilf(bw / TILE_SIZE_PX);
    l->tile_rows = (uint32_t)ceilf(bh / TILE_SIZE_PX);
    uint32_t total = l->tile_cols * l->tile_rows;
    if (total > MAX_TILES_PER_LAYER) total = MAX_TILES_PER_LAYER;
    l->tile_count = total;

    for (uint32_t i = 0; i < total; i++) {
        LayerTile *t = &l->tiles[i];
        t->tile_id = i;
        t->col = i % l->tile_cols;
        t->row = i / l->tile_cols;
        t->x = t->col * TILE_SIZE_PX;
        t->y = t->row * TILE_SIZE_PX;
        t->state = TILE_PENDING;
        t->is_dirty = true;
        t->is_visible = false;
        t->priority = 0;
    }
}

void comp_rasterize_tile(Layer *l, uint32_t tile_id) {
    if (!l || tile_id >= l->tile_count) return;
    LayerTile *t = &l->tiles[tile_id];
    if (t->state == TILE_READY || t->state == TILE_RASTERIZING) return;
    t->state = TILE_RASTERIZING;
    uint32_t w = (uint32_t)TILE_SIZE_PX;
    uint32_t h = (uint32_t)TILE_SIZE_PX;
    if (t->col == l->tile_cols - 1) w = (uint32_t)l->bounds[2] - t->x;
    if (t->row == l->tile_rows - 1) h = (uint32_t)l->bounds[3] - t->y;
    t->width = w; t->height = h;
    t->pixel_size = w * h * 4;
    t->pixels = (uint8_t *)realloc(t->pixels, t->pixel_size);
    if (t->pixels) {
        memset(t->pixels, 0x80, t->pixel_size);
        l->memory_usage += t->pixel_size;
    }
    t->state = TILE_READY;
    t->is_dirty = false;
    l->raster_count++;
}

void comp_rasterize_dirty(Compositor *c) {
    for (int32_t i = 0; i < c->layer_count; i++) {
        Layer *l = &c->layers[i];
        if (!l->visible) continue;
        for (uint32_t t = 0; t < l->tile_count; t++) {
            if (l->tiles[t].is_dirty) comp_rasterize_tile(l, t);
        }
    }
}

void comp_commit_frame(Compositor *c) {
    c->frame_counter++;
    uint64_t frame_raster = 0;
    for (int32_t i = 0; i < c->layer_count; i++) {
        Layer *l = &c->layers[i];
        if (!l->visible) continue;
        l->last_commit_ms = c->frame_counter;
        frame_raster += l->raster_count;
        l->raster_count = 0;
    }
    c->tiles_rasterized += frame_raster;
}

void comp_calculate_visible_tiles(Compositor *c, float scroll_x, float scroll_y) {
    for (int32_t i = 0; i < c->layer_count; i++) {
        Layer *l = &c->layers[i];
        if (!l->visible) continue;
        for (uint32_t t = 0; t < l->tile_count; t++) {
            float tx = (float)l->tiles[t].x - scroll_x;
            float ty = (float)l->tiles[t].y - scroll_y;
            l->tiles[t].is_visible =
                (tx + TILE_SIZE_PX > 0 && tx < c->viewport_width &&
                 ty + TILE_SIZE_PX > 0 && ty < c->viewport_height);
        }
    }
}

void comp_update_tile_priorities(Compositor *c) {
    for (int32_t i = 0; i < c->layer_count; i++) {
        Layer *l = &c->layers[i];
        for (uint32_t t = 0; t < l->tile_count; t++) {
            l->tiles[t].priority = l->tiles[t].is_visible ? 255 : 0;
        }
    }
}

void comp_evict_tiles(Compositor *c, uint64_t max_bytes) {
    uint64_t total = comp_get_total_memory(c);
    if (total <= max_bytes) return;
    for (int32_t i = c->layer_count - 1; i >= 0 && total > max_bytes; i--) {
        Layer *l = &c->layers[i];
        for (uint32_t t = 0; t < l->tile_count && total > max_bytes; t++) {
            if (l->tiles[t].state == TILE_READY && l->tiles[t].priority < 128) {
                free(l->tiles[t].pixels); l->tiles[t].pixels = NULL;
                total -= l->tiles[t].pixel_size;
                l->memory_usage -= l->tiles[t].pixel_size;
                l->tiles[t].state = TILE_EVICTED;
                l->tiles[t].pixel_size = 0;
            }
        }
    }
}

void comp_set_impl_side_painting(Compositor *c, bool enabled)    { c->impl_side_painting = enabled; }
void comp_set_gpu_raster(Compositor *c, bool enabled)            { c->gpu_rasterization = enabled; }
void comp_set_zero_copy(Compositor *c, bool enabled)             { c->zero_copy_enabled = enabled; }

bool comp_is_composited_anim(const Layer *l) {
    return l->is_composited &&
           (comp_has_reason(l, COMP_REASON_TRANSFORM_ANIM) ||
            comp_has_reason(l, COMP_REASON_OPACITY_ANIM) ||
            comp_has_reason(l, COMP_REASON_ANIMATION));
}

const char *comp_layer_type_name(LayerType type) {
    static const char *names[] = { "Root","Paint","Graphics","Video","IFrame","WebGL",
                                   "Scroll","Overlay","Transform","Opacity","Filter","Mask" };
    return (type <= 11) ? names[type] : "?";
}

const char *comp_reason_name(CompositingReason reason) {
    switch (reason) {
        case COMP_REASON_TRANSFORM_3D:     return "transform3D";
        case COMP_REASON_TRANSFORM_ANIM:   return "transformAnim";
        case COMP_REASON_OPACITY_ANIM:     return "opacityAnim";
        case COMP_REASON_FILTER_ANIM:      return "filterAnim";
        case COMP_REASON_VIDEO:            return "video";
        case COMP_REASON_IFRAME:           return "iframe";
        case COMP_REASON_WEBGL:            return "webgl";
        case COMP_REASON_SCROLL:           return "scroll";
        case COMP_REASON_WILL_CHANGE:      return "willChange";
        case COMP_REASON_OVERLAP:          return "overlap";
        case COMP_REASON_STACKING_CONTEXT: return "stackingContext";
        case COMP_REASON_FIXED_POSITION:   return "fixedPos";
        case COMP_REASON_ANIMATION:        return "animation";
        default: return "?";
    }
}

uint64_t comp_get_total_memory(const Compositor *c) {
    uint64_t total = 0;
    for (int32_t i = 0; i < c->layer_count; i++) total += c->layers[i].memory_usage;
    return total;
}

void comp_dump_layer_tree(const Compositor *c, int32_t idx, int32_t depth) {
    if (idx < 0 || idx >= c->layer_count) return;
    const Layer *l = &c->layers[idx];
    for (int32_t d = 0; d < depth; d++) printf("  ");
    printf("[L%lld] %s bounds=(%.0f,%.0f %.0fx%.0f) opacity=%.2f composited=%d reasons=%u tiles=%u mem=%llu\n",
           (unsigned long long)l->layer_id, comp_layer_type_name(l->type),
           l->bounds[0], l->bounds[1], l->bounds[2], l->bounds[3],
           l->opacity, l->is_composited, l->compositing_reasons,
           l->tile_count, (unsigned long long)l->memory_usage);
    for (int32_t ci = 0; ci < l->child_count; ci++)
        comp_dump_layer_tree(c, l->child_ids[ci], depth + 1);
}
