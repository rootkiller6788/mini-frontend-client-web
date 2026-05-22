#ifndef MINI_UI_INTERACTION_MODEL_H
#define MINI_UI_INTERACTION_MODEL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ===== Pointer/Touch Event Types ===== */
typedef enum {
    POINTER_EVENT_NONE       = 0,
    POINTER_EVENT_DOWN       = 1,   /* pointerdown / touchstart */
    POINTER_EVENT_MOVE       = 2,   /* pointermove / touchmove */
    POINTER_EVENT_UP         = 3,   /* pointerup / touchend */
    POINTER_EVENT_CANCEL     = 4,   /* pointercancel / touchcancel */
    POINTER_EVENT_OVER       = 5,   /* pointerover */
    POINTER_EVENT_OUT        = 6,   /* pointerout */
    POINTER_EVENT_ENTER      = 7,   /* pointerenter */
    POINTER_EVENT_LEAVE      = 8,   /* pointerleave */
} ui_pointer_event_type_t;

/* ===== Pointer Type ===== */
typedef enum {
    POINTER_TYPE_MOUSE   = 0,
    POINTER_TYPE_TOUCH   = 1,
    POINTER_TYPE_PEN     = 2,
    POINTER_TYPE_UNKNOWN = 3,
} ui_pointer_type_t;

/* ===== Pointer Button ===== */
typedef enum {
    POINTER_BUTTON_NONE    = -1,
    POINTER_BUTTON_PRIMARY = 0,
    POINTER_BUTTON_AUXILIARY = 1,
    POINTER_BUTTON_SECONDARY = 2,
} ui_pointer_button_t;

/* ===== Pointer Event Data ===== */
typedef struct {
    ui_pointer_event_type_t type;
    ui_pointer_type_t       pointer_type;
    ui_pointer_button_t     button;
    float                   client_x;
    float                   client_y;
    float                   page_x;
    float                   page_y;
    float                   screen_x;
    float                   screen_y;
    float                   movement_x;
    float                   movement_y;
    float                   pressure;      /* 0.0 - 1.0 */
    float                   tilt_x;        /* -90 to 90 */
    float                   tilt_y;        /* -90 to 90 */
    float                   width;         /* contact geometry */
    float                   height;
    int                     pointer_id;
    bool                    is_primary;
    bool                    ctrl_key;
    bool                    alt_key;
    bool                    shift_key;
    bool                    meta_key;
    int64_t                 timestamp_ms;
    void                   *target;
} ui_pointer_event_t;

/* ===== Gesture Types ===== */
typedef enum {
    GESTURE_NONE         = 0,
    GESTURE_TAP           = 1,
    GESTURE_DOUBLE_TAP   = 2,
    GESTURE_LONG_PRESS    = 3,
    GESTURE_SWIPE_LEFT    = 4,
    GESTURE_SWIPE_RIGHT   = 5,
    GESTURE_SWIPE_UP      = 6,
    GESTURE_SWIPE_DOWN    = 7,
    GESTURE_PINCH_IN      = 8,
    GESTURE_PINCH_OUT     = 9,
    GESTURE_ROTATE        = 10,
    GESTURE_PAN           = 11,
    GESTURE_FLICK         = 12,
    GESTURE_DRAG_START    = 13,
    GESTURE_DRAG_MOVE     = 14,
    GESTURE_DRAG_END      = 15,
    GESTURE_TWO_FINGER_TAP = 16,
    GESTURE_FORCE_TOUCH   = 17,
} ui_gesture_type_t;

/* ===== Gesture State ===== */
typedef enum {
    GESTURE_STATE_POSSIBLE = 0,
    GESTURE_STATE_BEGAN    = 1,
    GESTURE_STATE_CHANGED  = 2,
    GESTURE_STATE_ENDED    = 3,
    GESTURE_STATE_CANCELLED = 4,
    GESTURE_STATE_FAILED   = 5,
    GESTURE_STATE_RECOGNIZED = 6,
} ui_gesture_state_t;

/* ===== Gesture Data ===== */
typedef struct {
    ui_gesture_type_t  type;
    ui_gesture_state_t state;
    float              center_x;       /* centroid */
    float              center_y;
    float              delta_x;        /* displacement */
    float              delta_y;
    float              velocity_x;     /* px/ms */
    float              velocity_y;
    float              scale;          /* pinch scale 0.0+ */
    float              rotation;       /* degrees */
    float              pressure;
    int                pointer_count;
    int64_t            duration_ms;    /* gesture duration */
    float              distance;       /* total travel distance */
    ui_pointer_event_t start_event;
    ui_pointer_event_t current_event;
    ui_pointer_event_t previous_event;
} ui_gesture_data_t;

/* ===== Gesture Recognizer Configuration ===== */
typedef struct {
    float   tap_max_distance;        /* pixels, default 10 */
    int     tap_max_duration_ms;     /* milliseconds, default 300 */
    int     double_tap_interval_ms;  /* milliseconds, default 300 */
    int     long_press_duration_ms;  /* milliseconds, default 500 */
    float   long_press_max_distance; /* pixels, default 10 */
    float   swipe_min_distance;      /* pixels, default 50 */
    float   swipe_min_velocity;      /* px/ms, default 0.3 */
    float   pinch_min_scale;         /* default 0.1 */
    float   rotation_min_degrees;    /* default 1.0 */
    bool    prevent_default;         /* call preventDefault */
    bool    capture_phase;           /* use capture phase */
    int     max_pointer_count;       /* max simultaneous pointers */
} ui_gesture_config_t;

/* ===== Gesture Recognizer ===== */
typedef struct ui_gesture_recognizer_s {
    ui_gesture_config_t    config;
    ui_pointer_event_t    *active_pointers;
    int                    active_pointer_count;
    int                    active_pointer_capacity;
    ui_pointer_event_t     initial_touch;
    int64_t                touch_start_time;
    int                    tap_count;
    int64_t                last_tap_time;
    float                  initial_distance;  /* for pinch */
    float                  initial_angle;     /* for rotation */
    bool                   is_active;
    ui_gesture_type_t      last_recognized;
    float                  start_center_x;
    float                  start_center_y;
    void                  *user_data;
} ui_gesture_recognizer_t;

/* ===== Click Delay Model ===== */
typedef enum {
    CLICK_DELAY_DISABLED = 0,   /* 0ms - removed */
    CLICK_DELAY_REDUCED  = 1,   /* 50ms - reduced */
    CLICK_DELAY_LEGACY   = 2,   /* 300ms - legacy */
} ui_click_delay_mode_t;

typedef struct {
    ui_click_delay_mode_t mode;
    int                   custom_delay_ms;
    bool                  use_fastclick;     /* FastClick polyfill */
    bool                  use_touch_action;  /* CSS touch-action */
    float                 double_tap_zoom_threshold; /* zoom if > threshold */
} ui_click_delay_config_t;

/* ===== Cursor States ===== */
typedef enum {
    CURSOR_DEFAULT      = 0,
    CURSOR_POINTER      = 1,
    CURSOR_TEXT          = 2,
    CURSOR_MOVE          = 3,
    CURSOR_NOT_ALLOWED   = 4,
    CURSOR_GRAB          = 5,
    CURSOR_GRABBING      = 6,
    CURSOR_CROSSHAIR     = 7,
    CURSOR_WAIT          = 8,
    CURSOR_PROGRESS      = 9,
    CURSOR_HELP          = 10,
    CURSOR_ZOOM_IN       = 11,
    CURSOR_ZOOM_OUT      = 12,
    CURSOR_CELL          = 13,
    CURSOR_COL_RESIZE    = 14,
    CURSOR_ROW_RESIZE    = 15,
    CURSOR_VERTICAL_TEXT = 16,
    CURSOR_ALIAS         = 17,
    CURSOR_COPY          = 18,
    CURSOR_NONE          = 19,
    CURSOR_CONTEXT_MENU  = 20,
    CURSOR_EW_RESIZE     = 21,
    CURSOR_NS_RESIZE     = 22,
    CURSOR_NESW_RESIZE   = 23,
    CURSOR_NWSE_RESIZE   = 24,
} ui_cursor_t;

/* ===== Hover/Active State Config ===== */
typedef struct {
    bool     enable_hover;
    bool     enable_active;
    bool     enable_focus_visible;
    int      hover_delay_ms;       /* delay before showing hover (0 = instant) */
    int      active_duration_ms;   /* how long active state persists */
    bool     sticky_hover;         /* hover stays on touch devices */
    bool     use_hover_media_query;/* respect @media (hover: hover) */
} ui_hover_config_t;

/* ===== Drag and Drop Model ===== */
typedef enum {
    DRAG_EFFECT_NONE    = 0,
    DRAG_EFFECT_COPY    = 1,
    DRAG_EFFECT_MOVE    = 2,
    DRAG_EFFECT_LINK    = 3,
} ui_drag_effect_t;

typedef enum {
    DROP_EFFECT_NONE    = 0,
    DROP_EFFECT_COPY    = 1,
    DROP_EFFECT_MOVE    = 2,
    DROP_EFFECT_LINK    = 3,
} ui_drop_effect_t;

typedef enum {
    DRAG_SOURCE_STATE_IDLE      = 0,
    DRAG_SOURCE_STATE_DRAGGING  = 1,
    DRAG_SOURCE_STATE_COMPLETE  = 2,
} ui_drag_source_state_t;

typedef enum {
    DROP_TARGET_STATE_IDLE       = 0,
    DROP_TARGET_STATE_DRAG_OVER  = 1,
    DROP_TARGET_STATE_DRAG_ENTER = 2,
    DROP_TARGET_STATE_DRAG_LEAVE = 3,
    DROP_TARGET_STATE_DROP       = 4,
} ui_drop_target_state_t;

typedef struct {
    ui_drag_source_state_t state;
    ui_drag_effect_t       allowed_effects;
    ui_drag_effect_t       current_effect;
    const char            *data_type;   /* MIME type */
    void                  *data;
    int                    data_size;
    float                  origin_x;
    float                  origin_y;
    float                  offset_x;
    float                  offset_y;
    void                  *element;
    const char            *drag_image_url;
    int                    drag_image_x;
    int                    drag_image_y;
} ui_drag_source_t;

typedef struct {
    ui_drop_target_state_t state;
    ui_drop_effect_t       allowed_effects;
    const char            *accepted_types; /* comma-separated MIME types */
    bool                   is_over;
    int                    over_count;     /* nesting depth */
    void                  *element;
} ui_drop_target_t;

/* ===== Interaction Context (per element) ===== */
typedef struct {
    ui_hover_config_t        hover_config;
    ui_cursor_t              cursor;
    ui_cursor_t              hover_cursor;
    ui_cursor_t              active_cursor;
    ui_cursor_t              disabled_cursor;
    bool                     is_hovered;
    bool                     is_active;
    bool                     is_focused;
    bool                     is_disabled;
    ui_drag_source_t        *drag_source;
    ui_drop_target_t        *drop_target;
    ui_gesture_recognizer_t *gesture_recognizer;
} ui_interaction_context_t;

/* ===== Animation Curve for Interactions ===== */
typedef enum {
    INTERACTION_CURVE_LINEAR       = 0,
    INTERACTION_CURVE_EASE         = 1,
    INTERACTION_CURVE_EASE_IN      = 2,
    INTERACTION_CURVE_EASE_OUT     = 3,
    INTERACTION_CURVE_EASE_IN_OUT  = 4,
    INTERACTION_CURVE_SPRING       = 5,
    INTERACTION_CURVE_BOUNCE       = 6,
} ui_interaction_curve_t;

typedef struct {
    ui_interaction_curve_t curve;
    int                    duration_ms;
    float                  stiffness;   /* spring stiffness */
    float                  damping;     /* spring damping */
    float                  mass;        /* spring mass */
} ui_interaction_animation_t;

/* ===== Scroll Behavior ===== */
typedef enum {
    SCROLL_BEHAVIOR_AUTO     = 0,
    SCROLL_BEHAVIOR_SMOOTH   = 1,
    SCROLL_BEHAVIOR_INSTANT  = 2,
} ui_scroll_behavior_t;

/* ===== Overscroll Behavior ===== */
typedef enum {
    OVERSCROLL_AUTO     = 0,
    OVERSCROLL_CONTAIN  = 1,
    OVERSCROLL_NONE     = 2,
} ui_overscroll_behavior_t;

/* ===== Touch Action ===== */
typedef enum {
    TOUCH_ACTION_AUTO          = 0,
    TOUCH_ACTION_NONE          = 1,
    TOUCH_ACTION_PAN_X         = 2,
    TOUCH_ACTION_PAN_Y         = 3,
    TOUCH_ACTION_PAN_LEFT      = 4,
    TOUCH_ACTION_PAN_RIGHT     = 5,
    TOUCH_ACTION_PAN_UP        = 6,
    TOUCH_ACTION_PAN_DOWN      = 7,
    TOUCH_ACTION_PINCH_ZOOM    = 8,
    TOUCH_ACTION_MANIPULATION  = 9,
} ui_touch_action_t;

/* ===== API ===== */

/* Initialize interaction subsystem */
void ui_interaction_init(void);
void ui_interaction_shutdown(void);

/* --- Pointer Events --- */

/* Create a pointer event */
ui_pointer_event_t ui_pointer_event_create(
    ui_pointer_event_type_t type,
    float x, float y,
    ui_pointer_type_t pointer_type);

/* Get CSS cursor string from cursor enum */
const char* ui_cursor_to_css(ui_cursor_t cursor);

/* Get touch action CSS value */
const char* ui_touch_action_to_css(ui_touch_action_t action);

/* --- Gesture Recognition --- */

/* Create gesture recognizer with default config */
ui_gesture_recognizer_t* ui_gesture_recognizer_create(void);

/* Create gesture recognizer with custom config */
ui_gesture_recognizer_t* ui_gesture_recognizer_create_with_config(
    const ui_gesture_config_t *config);

/* Feed a pointer event into the recognizer; returns recognized gesture */
ui_gesture_type_t ui_gesture_feed_event(
    ui_gesture_recognizer_t *recognizer,
    const ui_pointer_event_t *event);

/* Get current gesture data */
const ui_gesture_data_t* ui_gesture_get_data(
    const ui_gesture_recognizer_t *recognizer);

/* Reset gesture recognizer */
void ui_gesture_reset(ui_gesture_recognizer_t *recognizer);

/* Free gesture recognizer */
void ui_gesture_recognizer_free(ui_gesture_recognizer_t *recognizer);

/* --- Click Delay Model --- */

/* Configure click delay behavior */
void ui_click_delay_configure(const ui_click_delay_config_t *config);

/* Get recommended touch-action based on click delay config */
ui_touch_action_t ui_click_delay_touch_action(void);

/* Generate meta viewport tag for click delay elimination */
const char* ui_click_delay_viewport_meta(void);

/* Generate touch-action CSS rule */
const char* ui_click_delay_touch_action_css(void);

/* --- Interaction Context --- */

/* Create interaction context */
ui_interaction_context_t* ui_interaction_context_create(void);

/* Update interaction context based on pointer event */
void ui_interaction_update(
    ui_interaction_context_t *ctx,
    const ui_pointer_event_t *event);

/* Compute appropriate cursor based on context */
ui_cursor_t ui_interaction_compute_cursor(
    const ui_interaction_context_t *ctx);

/* Generate CSS for hover/active states */
int ui_interaction_render_css(
    const ui_interaction_context_t *ctx,
    const char                    *selector,
    char *buffer, int buf_size);

/* Free interaction context */
void ui_interaction_context_free(ui_interaction_context_t *ctx);

/* --- Drag and Drop --- */

/* Create drag source */
ui_drag_source_t* ui_drag_source_create(
    ui_drag_effect_t allowed_effects,
    const char      *data_type,
    void            *data, int data_size);

/* Start dragging */
void ui_drag_source_start(
    ui_drag_source_t *source,
    float origin_x, float origin_y,
    void *element);

/* Update drag position */
void ui_drag_source_update(
    ui_drag_source_t *source,
    float current_x, float current_y);

/* End drag */
void ui_drag_source_end(ui_drag_source_t *source);

/* Cancel drag */
void ui_drag_source_cancel(ui_drag_source_t *source);

/* Free drag source */
void ui_drag_source_free(ui_drag_source_t *source);

/* Create drop target */
ui_drop_target_t* ui_drop_target_create(
    ui_drop_effect_t allowed_effects,
    const char      *accepted_types);

/* Handle drag enter */
void ui_drop_target_drag_enter(
    ui_drop_target_t *target,
    const ui_drag_source_t *source);

/* Handle drag over */
void ui_drop_target_drag_over(
    ui_drop_target_t *target,
    float x, float y);

/* Handle drag leave */
void ui_drop_target_drag_leave(ui_drop_target_t *target);

/* Handle drop */
bool ui_drop_target_drop(
    ui_drop_target_t *target,
    const ui_drag_source_t *source);

/* Check if drop is accepted */
bool ui_drop_target_accepts(
    const ui_drop_target_t *target,
    const ui_drag_source_t *source);

/* Free drop target */
void ui_drop_target_free(ui_drop_target_t *target);

/* --- Interaction Animation --- */

/* Create interaction animation */
ui_interaction_animation_t ui_interaction_animation_create(
    ui_interaction_curve_t curve,
    int duration_ms);

/* Compute animation value at time t (0.0-1.0) */
float ui_interaction_animation_value(
    const ui_interaction_animation_t *anim,
    float t);

/* Generate CSS transition string */
int ui_interaction_animation_to_css(
    const ui_interaction_animation_t *anim,
    const char *property,
    char *buffer, int buf_size);

/* --- Scroll --- */

/* Get CSS scroll-behavior value */
const char* ui_scroll_behavior_to_css(ui_scroll_behavior_t behavior);

/* Get CSS overscroll-behavior value */
const char* ui_overscroll_behavior_to_css(ui_overscroll_behavior_t behavior);

/* Generate scroll-related CSS */
int ui_scroll_css_generate(
    ui_scroll_behavior_t behavior,
    ui_overscroll_behavior_t overscroll_x,
    ui_overscroll_behavior_t overscroll_y,
    char *buffer, int buf_size);

#endif /* MINI_UI_INTERACTION_MODEL_H */
