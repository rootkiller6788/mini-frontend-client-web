#include "push_notif.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

void pn_init(pn_manager_t *mgr)
{
    if (!mgr) return;
    memset(mgr, 0, sizeof(*mgr));

    mgr->service           = PN_SERVICE_SIM;
    mgr->registered        = false;
    mgr->remote_enabled    = false;
    mgr->badge_count       = 0;
    mgr->pending_count     = 0;
    mgr->foreground_present= true;
}

int pn_register_device(pn_manager_t *mgr, pn_service_t service)
{
    if (!mgr) return -1;

    mgr->service = service;

    snprintf(mgr->device_token, sizeof(mgr->device_token),
             "sim_token_%s_%016llx",
             service == PN_SERVICE_APNS ? "apns" :
             service == PN_SERVICE_FCM  ? "fcm"  : "sim",
             (unsigned long long)(uintptr_t)mgr);

    mgr->registered = true;
    mgr->remote_enabled = true;

    if (mgr->token_cb) {
        mgr->token_cb(mgr->device_token, mgr->cb_ctx);
    }

    return 0;
}

void pn_set_token_callback(pn_manager_t *mgr, pn_token_callback cb, void *ctx)
{ if (mgr) { mgr->token_cb = cb; mgr->cb_ctx = ctx; } }

void pn_set_receive_callback(pn_manager_t *mgr, pn_receive_callback cb, void *ctx)
{ if (mgr) { mgr->receive_cb = cb; mgr->cb_ctx = ctx; } }

void pn_set_action_callback(pn_manager_t *mgr, pn_action_callback cb, void *ctx)
{ if (mgr) { mgr->action_cb = cb; mgr->cb_ctx = ctx; } }

void pn_set_deep_link_callback(pn_manager_t *mgr, pn_deep_link_callback cb, void *ctx)
{ if (mgr) { mgr->deep_link_cb = cb; mgr->cb_ctx = ctx; } }

void pn_set_error_callback(pn_manager_t *mgr, pn_error_callback cb, void *ctx)
{ if (mgr) { mgr->error_cb = cb; mgr->cb_ctx = ctx; } }

void pn_build_payload(pn_payload_t *payload, const char *title,
                       const char *body, const char *data)
{
    if (!payload) return;
    memset(payload, 0, sizeof(*payload));

    if (title) strncpy(payload->title, title, sizeof(payload->title) - 1);
    if (body)  strncpy(payload->body, body, sizeof(payload->body) - 1);
    if (data)  strncpy(payload->data, data, sizeof(payload->data) - 1);

    payload->badge  = -1;
    payload->silent = false;
    payload->content_available = false;
    payload->sound[0] = '\0';

    payload->presentation_count = 3;
    payload->presentation[0] = PN_PRESENT_ALERT;
    payload->presentation[1] = PN_PRESENT_BADGE;
    payload->presentation[2] = PN_PRESENT_SOUND;
}

void pn_set_badge(pn_payload_t *payload, int badge)
{ if (payload) payload->badge = badge; }

void pn_set_silent(pn_payload_t *payload, bool silent)
{
    if (payload) {
        payload->silent = silent;
        payload->content_available = silent;
    }
}

void pn_add_action(pn_payload_t *payload, const char *id,
                    const char *title, bool destructive, bool foreground)
{
    if (!payload || !id || payload->action_count >= PN_MAX_ACTIONS) return;

    pn_action_t *act = &payload->actions[payload->action_count++];
    strncpy(act->id, id, sizeof(act->id) - 1);
    strncpy(act->title, title, sizeof(act->title) - 1);
    act->destructive              = destructive;
    act->foreground               = foreground;
    act->authentication_required  = false;
}

void pn_set_deep_link(pn_notification_t *notif, const char *link)
{
    if (notif && link) {
        strncpy(notif->deep_link, link, sizeof(notif->deep_link) - 1);
    }
}

void pn_deliver_notification(pn_manager_t *mgr, const pn_notification_t *notif,
                              pn_app_state_t state)
{
    if (!mgr || !notif) return;

    if (state == PN_STATE_ACTIVE && !mgr->foreground_present && !notif->payload.silent) {
        pn_notification_t *stored = &mgr->pending[mgr->pending_count];
        memcpy(stored, notif, sizeof(*stored));
        stored->delivered = false;
        if (mgr->pending_count < 64) mgr->pending_count++;
        return;
    }

    if (mgr->receive_cb) {
        mgr->receive_cb(notif, state, mgr->cb_ctx);
    }

    if (!notif->payload.silent) {
        pn_notification_t *stored = &mgr->pending[mgr->pending_count];
        memcpy(stored, notif, sizeof(*stored));
        stored->delivered = true;
        if (mgr->pending_count < 64) mgr->pending_count++;
    }
}

int pn_send_local(pn_manager_t *mgr, const pn_payload_t *payload,
                   float delay_seconds)
{
    if (!mgr || !payload) return -1;
    (void)delay_seconds;

    pn_notification_t notif;
    memset(&notif, 0, sizeof(notif));
    notif.id = (int32_t)(mgr->pending_count + 1);
    memcpy(&notif.payload, payload, sizeof(*payload));
    notif.received_at = (int64_t)time(NULL);
    notif.read        = false;
    notif.delivered   = false;

    pn_deliver_notification(mgr, &notif, PN_STATE_ACTIVE);

    return notif.id;
}

void pn_set_badge_count(pn_manager_t *mgr, int count)
{
    if (mgr) mgr->badge_count = count;
}

int pn_get_badge_count(pn_manager_t *mgr)
{
    return mgr ? mgr->badge_count : 0;
}

void pn_handle_token(pn_manager_t *mgr, const char *token)
{
    if (!mgr || !token) return;

    strncpy(mgr->device_token, token, sizeof(mgr->device_token) - 1);
    mgr->registered = true;

    if (mgr->token_cb) {
        mgr->token_cb(mgr->device_token, mgr->cb_ctx);
    }
}

void pn_handle_silent_push(pn_manager_t *mgr, const char *data)
{
    if (!mgr || !data) return;

    pn_payload_t payload;
    memset(&payload, 0, sizeof(payload));
    strncpy(payload.data, data, sizeof(payload.data) - 1);
    payload.silent = true;
    payload.content_available = true;

    pn_notification_t notif;
    memset(&notif, 0, sizeof(notif));
    notif.id = (int32_t)(mgr->pending_count + 1);
    memcpy(&notif.payload, &payload, sizeof(payload));
    notif.received_at = (int64_t)time(NULL);

    if (mgr->receive_cb) {
        mgr->receive_cb(&notif, PN_STATE_BACKGROUND, mgr->cb_ctx);
    }
}

int pn_pending_count(pn_manager_t *mgr)
{
    return mgr ? mgr->pending_count : 0;
}

const pn_notification_t *pn_get_pending(pn_manager_t *mgr, int index)
{
    if (!mgr || index < 0 || index >= mgr->pending_count) return NULL;
    return &mgr->pending[index];
}

void pn_clear_all(pn_manager_t *mgr)
{
    if (mgr) {
        mgr->pending_count = 0;
        mgr->badge_count   = 0;
    }
}

void pn_set_foreground_present(pn_manager_t *mgr, bool present)
{
    if (mgr) mgr->foreground_present = present;
}

void pn_destroy(pn_manager_t *mgr)
{
    if (mgr) memset(mgr, 0, sizeof(*mgr));
}
