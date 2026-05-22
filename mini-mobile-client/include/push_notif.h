#ifndef PUSH_NOTIF_H
#define PUSH_NOTIF_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PN_MAX_TOKEN_LEN     256
#define PN_MAX_PAYLOAD_LEN   2048
#define PN_MAX_TITLE_LEN     128
#define PN_MAX_BODY_LEN      512
#define PN_MAX_ACTION_ID_LEN 64
#define PN_MAX_ACTIONS       8
#define PN_MAX_DEEP_LINK_LEN 512

typedef enum {
    PN_SERVICE_APNS = 0,
    PN_SERVICE_FCM  = 1,
    PN_SERVICE_SIM  = 2
} pn_service_t;

typedef enum {
    PN_PRESENT_ALERT     = 0,
    PN_PRESENT_BADGE     = 1,
    PN_PRESENT_SOUND     = 2,
    PN_PRESENT_BANNER    = 3,
    PN_PRESENT_LIST      = 4
} pn_presentation_t;

typedef enum {
    PN_STATE_ACTIVE     = 0,
    PN_STATE_BACKGROUND = 1,
    PN_STATE_TERMINATED = 2
} pn_app_state_t;

typedef struct pn_action {
    char id[PN_MAX_ACTION_ID_LEN];
    char title[PN_MAX_TITLE_LEN];
    bool destructive;
    bool authentication_required;
    bool foreground;
} pn_action_t;

typedef struct pn_payload {
    char     title[PN_MAX_TITLE_LEN];
    char     body[PN_MAX_BODY_LEN];
    int      badge;
    char     sound[64];
    char     category[64];
    char     thread_id[64];

    char     data[PN_MAX_PAYLOAD_LEN];

    bool     silent;
    bool     content_available;

    char     launch_image[128];

    pn_action_t   actions[PN_MAX_ACTIONS];
    int           action_count;

    pn_presentation_t presentation[4];
    int             presentation_count;
} pn_payload_t;

typedef struct pn_notification {
    int32_t      id;
    pn_payload_t payload;
    char         deep_link[PN_MAX_DEEP_LINK_LEN];
    int64_t      received_at;
    bool         read;
    bool         delivered;
} pn_notification_t;

typedef void (*pn_token_callback)(const char *token, void *ctx);
typedef void (*pn_receive_callback)(const pn_notification_t *notif, pn_app_state_t state, void *ctx);
typedef void (*pn_action_callback)(const char *action_id, const pn_notification_t *notif, void *ctx);
typedef void (*pn_deep_link_callback)(const char *deep_link, void *ctx);
typedef void (*pn_error_callback)(int error_code, const char *message, void *ctx);

typedef struct pn_manager {
    pn_service_t        service;

    char                device_token[PN_MAX_TOKEN_LEN];
    bool                registered;
    bool                remote_enabled;

    int32_t             badge_count;

    pn_notification_t   pending[64];
    int32_t             pending_count;

    pn_token_callback   token_cb;
    pn_receive_callback receive_cb;
    pn_action_callback  action_cb;
    pn_deep_link_callback deep_link_cb;
    pn_error_callback   error_cb;
    void               *cb_ctx;

    bool                foreground_present;
} pn_manager_t;

void    pn_init                     (pn_manager_t *mgr);

int     pn_register_device          (pn_manager_t *mgr, pn_service_t service);

void    pn_set_token_callback       (pn_manager_t *mgr, pn_token_callback cb, void *ctx);
void    pn_set_receive_callback     (pn_manager_t *mgr, pn_receive_callback cb, void *ctx);
void    pn_set_action_callback      (pn_manager_t *mgr, pn_action_callback cb, void *ctx);
void    pn_set_deep_link_callback   (pn_manager_t *mgr, pn_deep_link_callback cb, void *ctx);
void    pn_set_error_callback       (pn_manager_t *mgr, pn_error_callback cb, void *ctx);

void    pn_build_payload            (pn_payload_t *payload, const char *title,
                                     const char *body, const char *data);
void    pn_set_badge                (pn_payload_t *payload, int badge);
void    pn_set_silent               (pn_payload_t *payload, bool silent);
void    pn_add_action               (pn_payload_t *payload, const char *id,
                                     const char *title, bool destructive,
                                     bool foreground);
void    pn_set_deep_link            (pn_notification_t *notif, const char *link);

void    pn_deliver_notification     (pn_manager_t *mgr, const pn_notification_t *notif,
                                     pn_app_state_t state);

int     pn_send_local               (pn_manager_t *mgr, const pn_payload_t *payload,
                                     float delay_seconds);

void    pn_set_badge_count          (pn_manager_t *mgr, int count);
int     pn_get_badge_count          (pn_manager_t *mgr);

void    pn_handle_token             (pn_manager_t *mgr, const char *token);
void    pn_handle_silent_push       (pn_manager_t *mgr, const char *data);

int     pn_pending_count            (pn_manager_t *mgr);
const pn_notification_t *pn_get_pending(pn_manager_t *mgr, int index);

void    pn_clear_all                (pn_manager_t *mgr);

void    pn_set_foreground_present   (pn_manager_t *mgr, bool present);

void    pn_destroy                  (pn_manager_t *mgr);

#ifdef __cplusplus
}
#endif

#endif
