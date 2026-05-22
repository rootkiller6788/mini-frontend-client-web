#include "event_system.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

void event_init(Event *event, const char *type, uint32_t flags) {
    memset(event, 0, sizeof(*event));
    strncpy(event->type, type, EVENT_TYPE_MAX_LEN - 1);
    event->flags = flags;
    event->timestamp = (int64_t)time(NULL);
}

void event_set_data(Event *event, const char *data, int len) {
    if (len > EVENT_DATA_MAX_LEN) len = EVENT_DATA_MAX_LEN;
    memcpy(event->data, data, (size_t)len);
    event->data_len = len;
}

void event_prevent_default(Event *event) { event->default_prevented = 1; }
void event_stop_propagation(Event *event) { event->propagation_stopped = 1; }
void event_stop_immediate_propagation(Event *event) {
    event->propagation_stopped = 1;
    event->immediate_stopped = 1;
}

int event_has_flag(const Event *event, uint32_t flag) {
    return (event->flags & flag) != 0;
}

int event_type_is(const Event *event, const char *type) {
    return strcmp(event->type, type) == 0;
}

/* =================== EventTarget =================== */

void event_target_init(EventTarget *target, const char *id) {
    memset(target, 0, sizeof(*target));
    target->id = (char*)malloc(strlen(id) + 1);
    if (target->id) strcpy(target->id, id);
}

void event_target_destroy(EventTarget *target) {
    free(target->id);
    target->id = NULL;
}

void event_target_append_child(EventTarget *parent, EventTarget *child) {
    if (parent->child_count >= EVENT_MAX_TARGET_DEPTH) return;
    child->parent = parent;
    parent->children[parent->child_count++] = child;
}

void event_target_remove_child(EventTarget *parent, EventTarget *child) {
    int i;
    for (i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            child->parent = NULL;
            if (i < parent->child_count - 1)
                memmove(&parent->children[i], &parent->children[i + 1],
                        (size_t)(parent->child_count - i - 1) * sizeof(EventTarget*));
            parent->child_count--;
            return;
        }
    }
}

int event_add_listener(EventTarget *target, const char *type,
                        EventListener callback, void *user_data,
                        int use_capture, int once, int passive) {
    int i;
    /* Deduplicate */
    for (i = 0; i < target->listener_count; i++) {
        EventListenerEntry *e = &target->listeners[i];
        if (e->callback == callback &&
            e->use_capture == use_capture &&
            strcmp(e->type, type) == 0) {
            e->active = 1;
            e->once = once;
            e->passive = passive;
            e->user_data = user_data;
            return 1;
        }
    }
    if (target->listener_count >= EVENT_MAX_LISTENERS) return 0;
    {
        EventListenerEntry *e = &target->listeners[target->listener_count++];
        strncpy(e->type, type, EVENT_TYPE_MAX_LEN - 1);
        e->callback   = callback;
        e->user_data  = user_data;
        e->use_capture = use_capture;
        e->once       = once;
        e->passive    = passive;
        e->active     = 1;
    }
    return 1;
}

int event_remove_listener(EventTarget *target, const char *type,
                           EventListener callback, int use_capture) {
    int i;
    for (i = 0; i < target->listener_count; i++) {
        EventListenerEntry *e = &target->listeners[i];
        if (e->callback == callback &&
            e->use_capture == use_capture &&
            strcmp(e->type, type) == 0) {
            if (i < target->listener_count - 1)
                memmove(&target->listeners[i], &target->listeners[i + 1],
                        (size_t)(target->listener_count - i - 1) * sizeof(EventListenerEntry));
            target->listener_count--;
            return 1;
        }
    }
    return 0;
}

void event_build_propagation_path(EventTarget *target,
                                   EventTarget **path, int *path_len) {
    int len = 0;
    EventTarget *cur = target;
    while (cur && len < EVENT_PROPAGATION_PATH_MAX) {
        path[len++] = cur;
        cur = cur->parent;
    }
    *path_len = len;
}

static int dispatch_to_listeners(EventTarget *target, Event *event,
                                  EventPhase phase) {
    int i, any_prevented = 0;
    event->current_target = target;
    event->phase = phase;

    for (i = 0; i < target->listener_count; i++) {
        EventListenerEntry *e = &target->listeners[i];
        if (!e->active) continue;
        if (strcmp(e->type, event->type) != 0) continue;

        /* Capture phase: only capture listeners */
        if (phase == EVENT_PHASE_CAPTURING && !e->use_capture) continue;
        /* Bubble phase: only non-capture listeners */
        if (phase == EVENT_PHASE_BUBBLING && e->use_capture) continue;

        e->callback(event, e->user_data);

        if (e->passive) any_prevented = 1;

        if (e->once) {
            e->active = 0;
            event_remove_listener(target, e->type, e->callback, e->use_capture);
            i--;
        }

        if (event->immediate_stopped) break;
    }
    return any_prevented;
}

int event_dispatch(EventTarget *target, Event *event) {
    EventTarget *path[EVENT_PROPAGATION_PATH_MAX];
    int path_len, i;
    event->target = target;
    event->default_prevented = 0;
    event->propagation_stopped = 0;
    event->immediate_stopped = 0;

    event_build_propagation_path(target, path, &path_len);

    /* Capture phase (top -> target's parent) */
    event->phase = EVENT_PHASE_CAPTURING;
    for (i = path_len - 1; i > 0; i--) {
        if (event->propagation_stopped) return !event->default_prevented;
        dispatch_to_listeners(path[i], event, EVENT_PHASE_CAPTURING);
    }

    /* Target phase */
    if (!event->propagation_stopped) {
        dispatch_to_listeners(path[0], event, EVENT_PHASE_AT_TARGET);
    }

    /* Bubble phase (target's parent -> top) */
    if (event_has_flag(event, EVENT_FLAG_BUBBLES) && !event->propagation_stopped) {
        event->phase = EVENT_PHASE_BUBBLING;
        for (i = 1; i < path_len; i++) {
            if (event->propagation_stopped) break;
            dispatch_to_listeners(path[i], event, EVENT_PHASE_BUBBLING);
        }
    }

    return !event->default_prevented;
}

int event_dispatch_at(EventTarget *target, const char *type,
                       uint32_t flags, const char *data, int data_len) {
    Event ev;
    event_init(&ev, type, flags);
    if (data && data_len > 0)
        event_set_data(&ev, data, data_len);
    return event_dispatch(target, &ev);
}
