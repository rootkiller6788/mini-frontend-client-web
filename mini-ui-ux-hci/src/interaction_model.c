#include "interaction_model.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static ui_click_delay_config_t g_click_delay_config = {
    CLICK_DELAY_DISABLED, 0, true, true, 0.5f
};

void ui_interaction_init(void) {
    g_click_delay_config.mode = CLICK_DELAY_DISABLED;
    g_click_delay_config.custom_delay_ms = 0;
    g_click_delay_config.use_fastclick = true;
    g_click_delay_config.use_touch_action = true;
}

void ui_interaction_shutdown(void) {
    /* no-op */
}

ui_pointer_event_t ui_pointer_event_create(
    ui_pointer_event_type_t type, float x, float y,
    ui_pointer_type_t pointer_type)
{
    ui_pointer_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    ev.client_x = x;
    ev.client_y = y;
    ev.page_x = x;
    ev.page_y = y;
    ev.pointer_type = pointer_type;
    ev.is_primary = true;
    ev.pressure = (type == POINTER_EVENT_DOWN || type == POINTER_EVENT_MOVE) ? 0.5f : 0.0f;
    ev.timestamp_ms = 0;
    return ev;
}

const char* ui_cursor_to_css(ui_cursor_t cursor) {
    static const char *map[] = {
        "default", "pointer", "text", "move", "not-allowed",
        "grab", "grabbing", "crosshair", "wait", "progress",
        "help", "zoom-in", "zoom-out", "cell", "col-resize",
        "row-resize", "vertical-text", "alias", "copy", "none",
        "context-menu", "ew-resize", "ns-resize", "nesw-resize", "nwse-resize"
    };
    if (cursor <= CURSOR_NWSE_RESIZE) return map[cursor];
    return "default";
}

const char* ui_touch_action_to_css(ui_touch_action_t action) {
    static const char *map[] = {
        "auto", "none", "pan-x", "pan-y",
        "pan-left", "pan-right", "pan-up", "pan-down",
        "pinch-zoom", "manipulation"
    };
    if (action <= TOUCH_ACTION_MANIPULATION) return map[action];
    return "auto";
}

ui_gesture_recognizer_t* ui_gesture_recognizer_create(void) {
    ui_gesture_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.tap_max_distance = 10.0f;
    cfg.tap_max_duration_ms = 300;
    cfg.double_tap_interval_ms = 300;
    cfg.long_press_duration_ms = 500;
    cfg.long_press_max_distance = 10.0f;
    cfg.swipe_min_distance = 50.0f;
    cfg.swipe_min_velocity = 0.3f;
    cfg.pinch_min_scale = 0.1f;
    cfg.rotation_min_degrees = 1.0f;
    cfg.prevent_default = false;
    cfg.capture_phase = false;
    cfg.max_pointer_count = 4;
    return ui_gesture_recognizer_create_with_config(&cfg);
}

ui_gesture_recognizer_t* ui_gesture_recognizer_create_with_config(
    const ui_gesture_config_t *config)
{
    ui_gesture_recognizer_t *rec = (ui_gesture_recognizer_t*)calloc(1, sizeof(ui_gesture_recognizer_t));
    if (!rec) return NULL;
    rec->config = *config;
    rec->active_pointer_capacity = 4;
    rec->active_pointers = (ui_pointer_event_t*)calloc(
        (size_t)rec->active_pointer_capacity, sizeof(ui_pointer_event_t));
    rec->active_pointer_count = 0;
    rec->is_active = false;
    rec->last_recognized = GESTURE_NONE;
    return rec;
}

static void ensure_pointer_capacity(ui_gesture_recognizer_t *rec, int needed) {
    if (rec->active_pointer_capacity >= needed) return;
    int nc = rec->active_pointer_capacity * 2;
    if (nc < needed) nc = needed;
    ui_pointer_event_t *np = (ui_pointer_event_t*)realloc(
        rec->active_pointers, (size_t)nc * sizeof(ui_pointer_event_t));
    if (!np) return;
    rec->active_pointers = np;
    rec->active_pointer_capacity = nc;
}

static float distance_between(const ui_pointer_event_t *a, const ui_pointer_event_t *b) {
    float dx = a->client_x - b->client_x;
    float dy = a->client_y - b->client_y;
    return (float)sqrt(dx * dx + dy * dy);
}

ui_gesture_type_t ui_gesture_feed_event(
    ui_gesture_recognizer_t *rec, const ui_pointer_event_t *event)
{
    if (!rec || !event) return GESTURE_NONE;
    ui_gesture_type_t result = GESTURE_NONE;

    switch (event->type) {
        case POINTER_EVENT_DOWN:
            if (rec->active_pointer_count == 0) {
                rec->initial_touch = *event;
                rec->touch_start_time = event->timestamp_ms;
                rec->start_center_x = event->client_x;
                rec->start_center_y = event->client_y;
                rec->is_active = true;
                rec->tap_count = 0;
            }
            ensure_pointer_capacity(rec, rec->active_pointer_count + 1);
            rec->active_pointers[rec->active_pointer_count++] = *event;
            if (rec->active_pointer_count == 2) {
                rec->initial_distance = distance_between(
                    &rec->active_pointers[0], &rec->active_pointers[1]);
                rec->initial_angle = (float)atan2(
                    rec->active_pointers[1].client_y - rec->active_pointers[0].client_y,
                    rec->active_pointers[1].client_x - rec->active_pointers[0].client_x);
            }
            break;

        case POINTER_EVENT_MOVE:
            if (!rec->is_active) break;
            for (int i = 0; i < rec->active_pointer_count; i++) {
                if (rec->active_pointers[i].pointer_id == event->pointer_id) {
                    rec->active_pointers[i] = *event;
                    break;
                }
            }
            {
                float dist = distance_between(&rec->initial_touch, event);
                if (rec->active_pointer_count == 1) {
                    if (dist >= rec->config.swipe_min_distance &&
                        event->timestamp_ms - rec->touch_start_time > 0) {
                        float vel = dist / (float)(event->timestamp_ms - rec->touch_start_time);
                        if (vel >= rec->config.swipe_min_velocity) {
                            float dx = event->client_x - rec->initial_touch.client_x;
                            float dy = event->client_y - rec->initial_touch.client_y;
                            if (fabsf(dx) > fabsf(dy))
                                result = (dx > 0) ? GESTURE_SWIPE_RIGHT : GESTURE_SWIPE_LEFT;
                            else
                                result = (dy > 0) ? GESTURE_SWIPE_DOWN : GESTURE_SWIPE_UP;
                            rec->last_recognized = result;
                        }
                    }
                } else if (rec->active_pointer_count == 2) {
                    float cur_dist = distance_between(
                        &rec->active_pointers[0], &rec->active_pointers[1]);
                    if (rec->initial_distance > 0.001f) {
                        float scale = cur_dist / rec->initial_distance;
                        if (fabsf(scale - 1.0f) > rec->config.pinch_min_scale) {
                            result = (scale > 1.0f) ? GESTURE_PINCH_OUT : GESTURE_PINCH_IN;
                            rec->last_recognized = result;
                        }
                    }
                }
            }
            break;

        case POINTER_EVENT_UP:
            if (rec->is_active) {
                float dist = distance_between(&rec->initial_touch, event);
                int64_t dur = event->timestamp_ms - rec->touch_start_time;

                if (dist <= rec->config.tap_max_distance && dur <= (int64_t)rec->config.tap_max_duration_ms) {
                    rec->tap_count++;
                    int64_t last_interval = rec->touch_start_time - rec->last_tap_time;
                    if (rec->tap_count >= 2 && last_interval > 0 &&
                        last_interval <= (int64_t)rec->config.double_tap_interval_ms) {
                        result = GESTURE_DOUBLE_TAP;
                    } else {
                        result = GESTURE_TAP;
                    }
                    rec->last_tap_time = rec->touch_start_time;
                } else if (dur >= (int64_t)rec->config.long_press_duration_ms &&
                           dist <= rec->config.long_press_max_distance) {
                    result = GESTURE_LONG_PRESS;
                }

                rec->last_recognized = result;
                for (int i = 0; i < rec->active_pointer_count; i++) {
                    if (rec->active_pointers[i].pointer_id == event->pointer_id) {
                        if (i < rec->active_pointer_count - 1)
                            rec->active_pointers[i] = rec->active_pointers[rec->active_pointer_count - 1];
                        rec->active_pointer_count--;
                        break;
                    }
                }
                if (rec->active_pointer_count == 0) {
                    rec->is_active = false;
                    rec->tap_count = 0;
                }
            }
            break;

        case POINTER_EVENT_CANCEL:
            rec->is_active = false;
            rec->active_pointer_count = 0;
            rec->tap_count = 0;
            result = GESTURE_NONE;
            break;

        default: break;
    }

    return result;
}

const ui_gesture_data_t* ui_gesture_get_data(const ui_gesture_recognizer_t *recognizer) {
    (void)recognizer;
    return NULL;
}

void ui_gesture_reset(ui_gesture_recognizer_t *recognizer) {
    if (!recognizer) return;
    recognizer->active_pointer_count = 0;
    recognizer->is_active = false;
    recognizer->tap_count = 0;
    recognizer->last_recognized = GESTURE_NONE;
}

void ui_gesture_recognizer_free(ui_gesture_recognizer_t *recognizer) {
    if (!recognizer) return;
    free(recognizer->active_pointers);
    free(recognizer);
}

void ui_click_delay_configure(const ui_click_delay_config_t *config) {
    if (!config) return;
    g_click_delay_config = *config;
}

ui_touch_action_t ui_click_delay_touch_action(void) {
    return TOUCH_ACTION_MANIPULATION;
}

const char* ui_click_delay_viewport_meta(void) {
    return "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">";
}

const char* ui_click_delay_touch_action_css(void) {
    return "html { touch-action: manipulation; }";
}

ui_interaction_context_t* ui_interaction_context_create(void) {
    ui_interaction_context_t *ctx = (ui_interaction_context_t*)calloc(1, sizeof(ui_interaction_context_t));
    if (!ctx) return NULL;
    ctx->hover_config.enable_hover = true;
    ctx->hover_config.enable_active = true;
    ctx->hover_config.enable_focus_visible = true;
    ctx->hover_config.hover_delay_ms = 0;
    ctx->hover_config.active_duration_ms = 100;
    ctx->hover_config.sticky_hover = false;
    ctx->hover_config.use_hover_media_query = true;
    ctx->cursor = CURSOR_DEFAULT;
    ctx->hover_cursor = CURSOR_POINTER;
    ctx->active_cursor = CURSOR_POINTER;
    ctx->disabled_cursor = CURSOR_NOT_ALLOWED;
    return ctx;
}

void ui_interaction_update(ui_interaction_context_t *ctx, const ui_pointer_event_t *event) {
    if (!ctx || !event) return;
    switch (event->type) {
        case POINTER_EVENT_OVER:
        case POINTER_EVENT_ENTER:
            ctx->is_hovered = true;
            break;
        case POINTER_EVENT_OUT:
        case POINTER_EVENT_LEAVE:
            ctx->is_hovered = false;
            ctx->is_active = false;
            break;
        case POINTER_EVENT_DOWN:
            ctx->is_active = true;
            break;
        case POINTER_EVENT_UP:
            ctx->is_active = false;
            break;
        default: break;
    }
}

ui_cursor_t ui_interaction_compute_cursor(const ui_interaction_context_t *ctx) {
    if (!ctx) return CURSOR_DEFAULT;
    if (ctx->is_disabled) return ctx->disabled_cursor;
    if (ctx->is_active) return ctx->active_cursor;
    if (ctx->is_hovered) return ctx->hover_cursor;
    return ctx->cursor;
}

int ui_interaction_render_css(const ui_interaction_context_t *ctx,
                              const char *selector, char *buffer, int buf_size) {
    if (!ctx || !selector || !buffer) return 0;
    int pos = 0;

    pos += snprintf(buffer + pos, (size_t)(buf_size - pos), "%s {\n", selector);
    pos += snprintf(buffer + pos, (size_t)(buf_size - pos),
                    "  cursor: %s;\n", ui_cursor_to_css(ctx->cursor));
    pos += snprintf(buffer + pos, (size_t)(buf_size - pos), "}\n");

    if (ctx->hover_config.enable_hover) {
        pos += snprintf(buffer + pos, (size_t)(buf_size - pos),
                        "%s:hover {\n  cursor: %s;\n}\n",
                        selector, ui_cursor_to_css(ctx->hover_cursor));
    }
    if (ctx->hover_config.enable_active) {
        pos += snprintf(buffer + pos, (size_t)(buf_size - pos),
                        "%s:active {\n  cursor: %s;\n}\n",
                        selector, ui_cursor_to_css(ctx->active_cursor));
    }
    if (ctx->is_disabled) {
        pos += snprintf(buffer + pos, (size_t)(buf_size - pos),
                        "%s[disabled], %s.disabled {\n  cursor: %s; opacity: 0.5;\n}\n",
                        selector, selector, ui_cursor_to_css(ctx->disabled_cursor));
    }
    if (ctx->hover_config.enable_focus_visible) {
        pos += snprintf(buffer + pos, (size_t)(buf_size - pos),
                        "%s:focus-visible {\n  outline: 2px solid var(--color-blue-500);\n  outline-offset: 2px;\n}\n",
                        selector);
    }

    return pos;
}

void ui_interaction_context_free(ui_interaction_context_t *ctx) {
    if (!ctx) return;
    if (ctx->drag_source) ui_drag_source_free(ctx->drag_source);
    if (ctx->drop_target) ui_drop_target_free(ctx->drop_target);
    if (ctx->gesture_recognizer) ui_gesture_recognizer_free(ctx->gesture_recognizer);
    free(ctx);
}

/* --- Drag and Drop --- */

ui_drag_source_t* ui_drag_source_create(ui_drag_effect_t allowed_effects,
                                        const char *data_type,
                                        void *data, int data_size) {
    ui_drag_source_t *ds = (ui_drag_source_t*)calloc(1, sizeof(ui_drag_source_t));
    if (!ds) return NULL;
    ds->allowed_effects = allowed_effects;
    ds->data_type = data_type;
    ds->data = data;
    ds->data_size = data_size;
    ds->state = DRAG_SOURCE_STATE_IDLE;
    return ds;
}

void ui_drag_source_start(ui_drag_source_t *source, float origin_x, float origin_y, void *element) {
    if (!source) return;
    source->state = DRAG_SOURCE_STATE_DRAGGING;
    source->origin_x = origin_x;
    source->origin_y = origin_y;
    source->element = element;
}

void ui_drag_source_update(ui_drag_source_t *source, float current_x, float current_y) {
    if (!source || source->state != DRAG_SOURCE_STATE_DRAGGING) return;
    source->offset_x = current_x - source->origin_x;
    source->offset_y = current_y - source->origin_y;
}

void ui_drag_source_end(ui_drag_source_t *source) {
    if (!source) return;
    source->state = DRAG_SOURCE_STATE_COMPLETE;
}

void ui_drag_source_cancel(ui_drag_source_t *source) {
    if (!source) return;
    source->state = DRAG_SOURCE_STATE_IDLE;
}

void ui_drag_source_free(ui_drag_source_t *source) { free(source); }

ui_drop_target_t* ui_drop_target_create(ui_drop_effect_t allowed_effects,
                                        const char *accepted_types) {
    ui_drop_target_t *dt = (ui_drop_target_t*)calloc(1, sizeof(ui_drop_target_t));
    if (!dt) return NULL;
    dt->allowed_effects = allowed_effects;
    dt->accepted_types = accepted_types;
    dt->state = DROP_TARGET_STATE_IDLE;
    return dt;
}

void ui_drop_target_drag_enter(ui_drop_target_t *target, const ui_drag_source_t *source) {
    if (!target || !source) return;
    target->state = DROP_TARGET_STATE_DRAG_ENTER;
    target->is_over = true;
    target->over_count++;
}

void ui_drop_target_drag_over(ui_drop_target_t *target, float x, float y) {
    if (!target) return;
    target->state = DROP_TARGET_STATE_DRAG_OVER;
    (void)x; (void)y;
}

void ui_drop_target_drag_leave(ui_drop_target_t *target) {
    if (!target) return;
    target->over_count--;
    if (target->over_count <= 0) {
        target->is_over = false;
        target->over_count = 0;
        target->state = DROP_TARGET_STATE_DRAG_LEAVE;
    }
}

bool ui_drop_target_drop(ui_drop_target_t *target, const ui_drag_source_t *source) {
    if (!target || !source) return false;
    target->state = DROP_TARGET_STATE_DROP;
    target->is_over = false;
    target->over_count = 0;
    return true;
}

bool ui_drop_target_accepts(const ui_drop_target_t *target, const ui_drag_source_t *source) {
    if (!target || !source) return false;
    if (!target->accepted_types) return true;
    if (!source->data_type) return false;
    if (strstr(target->accepted_types, source->data_type) != NULL) return true;
    return false;
}

void ui_drop_target_free(ui_drop_target_t *target) { free(target); }

/* --- Interaction Animation --- */

ui_interaction_animation_t ui_interaction_animation_create(
    ui_interaction_curve_t curve, int duration_ms)
{
    ui_interaction_animation_t anim;
    memset(&anim, 0, sizeof(anim));
    anim.curve = curve;
    anim.duration_ms = duration_ms;
    anim.stiffness = 200.0f;
    anim.damping = 20.0f;
    anim.mass = 1.0f;
    return anim;
}

float ui_interaction_animation_value(const ui_interaction_animation_t *anim, float t) {
    if (!anim) return t;
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    switch (anim->curve) {
        case INTERACTION_CURVE_LINEAR: return t;
        case INTERACTION_CURVE_EASE: return t < 0.5f ? 4.0f * t * t * t : 1.0f - (float)pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
        case INTERACTION_CURVE_EASE_IN: return t * t;
        case INTERACTION_CURVE_EASE_OUT: return 1.0f - (1.0f - t) * (1.0f - t);
        case INTERACTION_CURVE_EASE_IN_OUT: return t < 0.5f ? 2.0f * t * t : 1.0f - (float)pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
        default: return t;
    }
}

int ui_interaction_animation_to_css(const ui_interaction_animation_t *anim,
                                    const char *property, char *buffer, int buf_size) {
    if (!anim || !buffer) return 0;
    static const char *curve_css[] = {
        "linear", "ease", "ease-in", "ease-out", "ease-in-out",
        "cubic-bezier(0.34,1.56,0.64,1)", "cubic-bezier(0.68,-0.55,0.27,1.55)"
    };
    return snprintf(buffer, (size_t)buf_size,
        "transition: %s %dms %s;",
        property ? property : "all",
        anim->duration_ms,
        curve_css[anim->curve <= INTERACTION_CURVE_BOUNCE ? anim->curve : 0]);
}

const char* ui_scroll_behavior_to_css(ui_scroll_behavior_t behavior) {
    static const char *map[] = {"auto", "smooth", "auto"};
    if (behavior <= SCROLL_BEHAVIOR_INSTANT) return map[behavior];
    return "auto";
}

const char* ui_overscroll_behavior_to_css(ui_overscroll_behavior_t behavior) {
    static const char *map[] = {"auto", "contain", "none"};
    if (behavior <= OVERSCROLL_NONE) return map[behavior];
    return "auto";
}

int ui_scroll_css_generate(ui_scroll_behavior_t behavior,
                           ui_overscroll_behavior_t overscroll_x,
                           ui_overscroll_behavior_t overscroll_y,
                           char *buffer, int buf_size) {
    if (!buffer) return 0;
    return snprintf(buffer, (size_t)buf_size,
        "scroll-behavior: %s;\n"
        "overscroll-behavior-x: %s;\n"
        "overscroll-behavior-y: %s;",
        ui_scroll_behavior_to_css(behavior),
        ui_overscroll_behavior_to_css(overscroll_x),
        ui_overscroll_behavior_to_css(overscroll_y));
}
