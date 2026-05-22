#include "event_system.h"
#include <stdio.h>
#include <string.h>

static void on_click(const Event *event, void *user_data) {
    const char *id = (const char*)user_data;
    printf("  [%s] %s on '%s' (phase=%d, bubbles=%d, cancelled=%d)\n",
           id, event->type,
           event->current_target ? ((EventTarget*)event->current_target)->id : "?",
           event->phase,
           event_has_flag(event, EVENT_FLAG_BUBBLES),
           event->default_prevented);
}

static void on_keydown(const Event *event, void *user_data) {
    printf("  [keydown] key data: %.*s (phase=%d)\n",
           event->data_len, event->data, event->phase);
    (void)user_data;
}

static void on_scroll_capture(const Event *event, void *user_data) {
    printf("  [capture] scroll on '%s'\n",
           ((EventTarget*)event->current_target)->id);
    (void)user_data;
}

static void cancel_handler(const Event *event, void *user_data) {
    printf("  [cancel] Stopping propagation on '%s'\n",
           ((EventTarget*)event->current_target)->id);
    event_stop_propagation((Event*)event);
    (void)user_data;
}

int main(void) {
    printf("=== Event System Demo ===\n\n");

    /* Build a DOM-like tree:
     *   document
     *     └─ body
     *          ├─ button
     *          └─ div
     *               └─ span
     */
    EventTarget document, body, button, div, span;

    event_target_init(&document, "document");
    event_target_init(&body, "body");
    event_target_init(&button, "button");
    event_target_init(&div, "div");
    event_target_init(&span, "span");

    event_target_append_child(&document, &body);
    event_target_append_child(&body, &button);
    event_target_append_child(&body, &div);
    event_target_append_child(&div, &span);

    /* Register listeners */
    event_add_listener(&document, EVENT_TYPE_CLICK, on_click,
                       "doc-bubble", 0, 0, 0);          /* bubble */
    event_add_listener(&document, EVENT_TYPE_CLICK, on_click,
                       "doc-capture", 1, 0, 0);          /* capture */
    event_add_listener(&body, EVENT_TYPE_CLICK, on_click,
                       "body-bubble", 0, 0, 0);
    event_add_listener(&button, EVENT_TYPE_CLICK, on_click,
                       "btn-bubble", 0, 0, 0);
    event_add_listener(&span, EVENT_TYPE_CLICK, on_click,
                       "span-bubble", 0, 0, 0);

    event_add_listener(&div, EVENT_TYPE_KEYDOWN, on_keydown, NULL, 0, 0, 0);

    event_add_listener(&document, EVENT_TYPE_SCROLL, on_scroll_capture,
                       NULL, 1, 0, 0);

    /* --- Test 1: Click on button (full capture + bubble) --- */
    printf("--- Test 1: Click on 'button' ---\n");
    {
        Event ev;
        event_init(&ev, EVENT_TYPE_CLICK, EVENT_FLAG_BUBBLES);
        ev.data_len = (int)strlen("{\"x\":10,\"y\":20}");
        memcpy(ev.data, "{\"x\":10,\"y\":20}", ev.data_len);
        event_dispatch(&button, &ev);
    }

    /* --- Test 2: Click on span (deeper nested) --- */
    printf("\n--- Test 2: Click on 'span' ---\n");
    event_dispatch_at(&span, EVENT_TYPE_CLICK, EVENT_FLAG_BUBBLES, NULL, 0);

    /* --- Test 3: Keydown with data --- */
    printf("\n--- Test 3: Keydown on 'div' ---\n");
    event_dispatch_at(&div, EVENT_TYPE_KEYDOWN,
                      EVENT_FLAG_BUBBLES, "Enter", 5);

    /* --- Test 4: Scroll (capture) --- */
    printf("\n--- Test 4: Scroll on 'body' ---\n");
    event_dispatch_at(&body, EVENT_TYPE_SCROLL, EVENT_FLAG_NONE, NULL, 0);

    /* --- Test 5: Propagation stopped --- */
    printf("\n--- Test 5: Stop propagation at 'body' ---\n");
    {
        int idx;
        /* Remove old bubble listener and add cancel handler */
        for (idx = 0; idx < body.listener_count; idx++) {
            if (body.listeners[idx].callback == on_click) {
                body.listeners[idx].active = 0;
                body.listeners[idx].callback = cancel_handler;
                body.listeners[idx].active = 1;
                break;
            }
        }
        event_dispatch(&button, &(Event){
            .type = EVENT_TYPE_CLICK,  /* expected but ok for illustration */
        });
        /* Restore */
        for (idx = 0; idx < body.listener_count; idx++) {
            if (body.listeners[idx].callback == cancel_handler) {
                body.listeners[idx].callback = on_click;
                break;
            }
        }
    }

    /* --- Test 6: Custom event --- */
    printf("\n--- Test 6: Custom 'form-submit' event ---\n");
    event_add_listener(&div, "form-submit", on_click, "custom-div", 0, 0, 0);
    event_dispatch_at(&div, "form-submit", EVENT_FLAG_BUBBLES,
                      "custom-data", 11);

    /* Cleanup */
    event_target_destroy(&span);
    event_target_destroy(&div);
    event_target_destroy(&button);
    event_target_destroy(&body);
    event_target_destroy(&document);

    printf("\nDone.\n");
    return 0;
}
