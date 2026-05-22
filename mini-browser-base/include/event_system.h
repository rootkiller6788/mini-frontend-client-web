#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

#include <stdint.h>
#include <stddef.h>

#define EVENT_TYPE_MAX_LEN      64
#define EVENT_MAX_LISTENERS     128
#define EVENT_MAX_TARGET_DEPTH  32
#define EVENT_PROPAGATION_PATH_MAX 64
#define EVENT_DATA_MAX_LEN      4096

typedef enum {
    EVENT_PHASE_NONE         = 0,
    EVENT_PHASE_CAPTURING    = 1,
    EVENT_PHASE_AT_TARGET    = 2,
    EVENT_PHASE_BUBBLING     = 3
} EventPhase;

typedef enum {
    EVENT_FLAG_NONE          = 0,
    EVENT_FLAG_BUBBLES       = 1 << 0,
    EVENT_FLAG_CANCELABLE    = 1 << 1,
    EVENT_FLAG_COMPOSED      = 1 << 2,
    EVENT_FLAG_TRUSTED       = 1 << 3
} EventFlag;

/* Predefined event types */
#define EVENT_TYPE_CLICK     "click"
#define EVENT_TYPE_KEYDOWN   "keydown"
#define EVENT_TYPE_KEYUP     "keyup"
#define EVENT_TYPE_SCROLL    "scroll"
#define EVENT_TYPE_MOUSEOVER "mouseover"
#define EVENT_TYPE_MOUSEOUT  "mouseout"
#define EVENT_TYPE_FOCUS     "focus"
#define EVENT_TYPE_BLUR      "blur"
#define EVENT_TYPE_SUBMIT    "submit"
#define EVENT_TYPE_RESIZE    "resize"
#define EVENT_TYPE_LOAD      "load"
#define EVENT_TYPE_UNLOAD    "unload"
#define EVENT_TYPE_CHANGE    "change"
#define EVENT_TYPE_INPUT     "input"
#define EVENT_TYPE_CUSTOM    "custom"

typedef struct {
    char         type[EVENT_TYPE_MAX_LEN];
    void        *target;
    void        *current_target;
    EventPhase   phase;
    uint32_t     flags;
    int64_t      timestamp;
    int          default_prevented;
    int          propagation_stopped;
    int          immediate_stopped;
    char         data[EVENT_DATA_MAX_LEN];
    int          data_len;
} Event;

typedef void (*EventListener)(const Event *event, void *user_data);

typedef struct {
    char            type[EVENT_TYPE_MAX_LEN];
    EventListener   callback;
    void           *user_data;
    int             use_capture;
    int             once;
    int             passive;
    int             active;
} EventListenerEntry;

typedef struct EventTarget EventTarget;

struct EventTarget {
    EventListenerEntry listeners[EVENT_MAX_LISTENERS];
    int                listener_count;

    char              *id;
    EventTarget       *parent;
    EventTarget       *children[EVENT_MAX_TARGET_DEPTH];
    int                child_count;

    void              *user_data;
};

void  event_init(Event *event, const char *type, uint32_t flags);
void  event_set_data(Event *event, const char *data, int len);

void  event_prevent_default(Event *event);
void  event_stop_propagation(Event *event);
void  event_stop_immediate_propagation(Event *event);

void  event_target_init(EventTarget *target, const char *id);
void  event_target_destroy(EventTarget *target);

void  event_target_append_child(EventTarget *parent, EventTarget *child);
void  event_target_remove_child(EventTarget *parent, EventTarget *child);

int   event_add_listener(EventTarget *target, const char *type,
                         EventListener callback, void *user_data,
                         int use_capture, int once, int passive);

int   event_remove_listener(EventTarget *target, const char *type,
                            EventListener callback, int use_capture);

int   event_dispatch(EventTarget *target, Event *event);

int   event_dispatch_at(EventTarget *target, const char *type,
                        uint32_t flags, const char *data, int data_len);

void  event_build_propagation_path(EventTarget *target,
                                   EventTarget **path, int *path_len);

/* Inline helpers */
int   event_has_flag(const Event *event, uint32_t flag);
int   event_type_is(const Event *event, const char *type);

#endif
