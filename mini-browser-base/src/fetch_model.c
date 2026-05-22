#include "fetch_model.h"
#include <stdlib.h>
#include <string.h>

/* =================== AbortController =================== */

void abort_controller_init(AbortController *ctrl) {
    memset(ctrl, 0, sizeof(*ctrl));
}

void abort_controller_abort(AbortController *ctrl) {
    ctrl->signal.aborted = 1;
}

int abort_signal_aborted(const AbortSignal *signal) {
    return signal ? signal->aborted : 0;
}

/* =================== CORS =================== */

void cors_config_init(CorsConfig *cfg, const char *origin,
                      const char *allowed_origins, int allow_credentials) {
    memset(cfg, 0, sizeof(*cfg));
    if (origin) strncpy(cfg->origin, origin, FETCH_URL_MAX_LEN - 1);
    if (allowed_origins) strncpy(cfg->allowed_origins, allowed_origins, FETCH_URL_MAX_LEN - 1);
    cfg->allow_credentials = allow_credentials;
}

static int origin_matches(const char *allowed, const char *request_origin) {
    if (!allowed || !request_origin) return 0;
    if (strcmp(allowed, "*") == 0) return 1;
    return strcmp(allowed, request_origin) == 0;
}

CorsResult cors_check_simple(const FetchRequest *req, const CorsConfig *cfg) {
    char origin[FETCH_URL_MAX_LEN] = "";
    int i;

    for (i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].name, "origin") == 0) {
            strncpy(origin, req->headers[i].value, FETCH_URL_MAX_LEN - 1);
            break;
        }
    }

    if (!origin[0]) return CORS_ALLOWED; /* Same-origin, no Origin header */

    if (!origin_matches(cfg->allowed_origins, origin))
        return CORS_BLOCKED;

    return CORS_ALLOWED;
}

CorsResult cors_check_preflight(const FetchRequest *req,
                                 const CorsConfig *cfg,
                                 const char *req_method,
                                 const char *req_headers) {
    (void)req_method;  /* In simple sim, we check the config */
    (void)req_headers;

    {
        char origin[FETCH_URL_MAX_LEN] = "";
        int i;
        for (i = 0; i < req->header_count; i++) {
            if (strcasecmp(req->headers[i].name, "origin") == 0) {
                strncpy(origin, req->headers[i].value, FETCH_URL_MAX_LEN - 1);
                break;
            }
        }
        if (!origin[0]) return CORS_ALLOWED;
        if (!origin_matches(cfg->allowed_origins, origin))
            return CORS_BLOCKED;
    }
    return CORS_ALLOWED;
}

/* =================== Interceptor Chain =================== */

void fetch_interceptor_chain_init(FetchInterceptorChain *chain) {
    memset(chain, 0, sizeof(*chain));
}

int fetch_interceptor_chain_add(FetchInterceptorChain *chain,
                                 FetchInterceptor cb, void *user_data) {
    if (chain->count >= FETCH_MAX_INTERCEPTORS) return 0;
    chain->entries[chain->count].callback  = cb;
    chain->entries[chain->count].user_data = user_data;
    chain->entries[chain->count].active    = 1;
    chain->count++;
    return 1;
}

int fetch_interceptor_chain_remove(FetchInterceptorChain *chain,
                                    FetchInterceptor cb) {
    int i;
    for (i = 0; i < chain->count; i++) {
        if (chain->entries[i].callback == cb) {
            if (i < chain->count - 1)
                memmove(&chain->entries[i], &chain->entries[i + 1],
                        (size_t)(chain->count - i - 1) * sizeof(FetchInterceptorEntry));
            chain->count--;
            return 1;
        }
    }
    return 0;
}

FetchResponse *fetch_interceptor_chain_run(FetchInterceptorChain *chain,
                                            const FetchRequest *req) {
    int i;
    for (i = 0; i < chain->count; i++) {
        FetchInterceptorEntry *e = &chain->entries[i];
        FetchResponse *resp;
        if (!e->active) continue;
        resp = e->callback(req, e->user_data);
        if (resp) return resp;
    }
    return NULL;
}

/* =================== Request =================== */

void fetch_request_init(FetchRequest *req, const char *method,
                        const char *url) {
    memset(req, 0, sizeof(*req));
    strncpy(req->method, method, FETCH_METHOD_MAX_LEN - 1);
    strncpy(req->url, url, FETCH_URL_MAX_LEN - 1);
    req->mode        = FETCH_MODE_CORS;
    req->credentials = FETCH_CREDENTIALS_SAME_ORIGIN;
    req->redirect    = FETCH_REDIRECT_FOLLOW;
}

void fetch_request_destroy(FetchRequest *req) {
    free(req->body);
    req->body = NULL;
}

int fetch_request_set_header(FetchRequest *req, const char *name,
                              const char *value) {
    int i;

    /* Replace existing header with same name */
    for (i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].name, name) == 0) {
            strncpy(req->headers[i].value, value, FETCH_HEADER_VALUE_LEN - 1);
            return 1;
        }
    }

    if (req->header_count >= FETCH_MAX_HEADERS) return 0;
    strncpy(req->headers[req->header_count].name, name, FETCH_HEADER_NAME_LEN - 1);
    strncpy(req->headers[req->header_count].value, value, FETCH_HEADER_VALUE_LEN - 1);
    req->header_count++;
    return 1;
}

int fetch_request_get_header(const FetchRequest *req, const char *name,
                              char *value_out, size_t max_len) {
    int i;
    for (i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].name, name) == 0) {
            strncpy(value_out, req->headers[i].value, max_len - 1);
            value_out[max_len - 1] = '\0';
            return 1;
        }
    }
    return 0;
}

void fetch_request_set_body(FetchRequest *req, const char *body, size_t len) {
    free(req->body);
    req->body = NULL;
    if (body && len > 0) {
        req->body = (char*)malloc(len + 1);
        if (req->body) {
            memcpy(req->body, body, len);
            req->body[len] = '\0';
            req->body_len = len;
        }
    }
}

/* =================== Response =================== */

void fetch_response_init(FetchResponse *resp, int status,
                          const char *status_text) {
    memset(resp, 0, sizeof(*resp));
    resp->status = status;
    if (status_text)
        strncpy(resp->status_text, status_text, FETCH_STATUS_TEXT_LEN - 1);
    else {
        switch (status) {
            case 200: strcpy(resp->status_text, "OK"); break;
            case 201: strcpy(resp->status_text, "Created"); break;
            case 204: strcpy(resp->status_text, "No Content"); break;
            case 301: strcpy(resp->status_text, "Moved Permanently"); break;
            case 302: strcpy(resp->status_text, "Found"); break;
            case 304: strcpy(resp->status_text, "Not Modified"); break;
            case 400: strcpy(resp->status_text, "Bad Request"); break;
            case 401: strcpy(resp->status_text, "Unauthorized"); break;
            case 403: strcpy(resp->status_text, "Forbidden"); break;
            case 404: strcpy(resp->status_text, "Not Found"); break;
            case 500: strcpy(resp->status_text, "Internal Server Error"); break;
            default:  snprintf(resp->status_text, FETCH_STATUS_TEXT_LEN, "%d", status); break;
        }
    }
    resp->ok = (status >= 200 && status < 300);
    strcpy(resp->type, "basic");
}

void fetch_response_destroy(FetchResponse *resp) {
    free(resp->body);
    resp->body = NULL;
}

int fetch_response_set_header(FetchResponse *resp, const char *name,
                               const char *value) {
    int i;
    for (i = 0; i < resp->header_count; i++) {
        if (strcasecmp(resp->headers[i].name, name) == 0) {
            strncpy(resp->headers[i].value, value, FETCH_HEADER_VALUE_LEN - 1);
            return 1;
        }
    }
    if (resp->header_count >= FETCH_MAX_HEADERS) return 0;
    strncpy(resp->headers[resp->header_count].name, name, FETCH_HEADER_NAME_LEN - 1);
    strncpy(resp->headers[resp->header_count].value, value, FETCH_HEADER_VALUE_LEN - 1);
    resp->header_count++;
    return 1;
}

int fetch_response_get_header(const FetchResponse *resp, const char *name,
                               char *value_out, size_t max_len) {
    int i;
    for (i = 0; i < resp->header_count; i++) {
        if (strcasecmp(resp->headers[i].name, name) == 0) {
            strncpy(value_out, resp->headers[i].value, max_len - 1);
            value_out[max_len - 1] = '\0';
            return 1;
        }
    }
    return 0;
}

void fetch_response_set_body(FetchResponse *resp, const char *body, size_t len) {
    free(resp->body);
    resp->body = NULL;
    if (body && len > 0) {
        resp->body = (char*)malloc(len + 1);
        if (resp->body) {
            memcpy(resp->body, body, len);
            resp->body[len] = '\0';
            resp->body_len = len;
        }
    }
}

FetchResponse *fetch_response_clone(const FetchResponse *resp) {
    FetchResponse *clone = (FetchResponse*)malloc(sizeof(FetchResponse));
    if (!clone) return NULL;
    memcpy(clone, resp, sizeof(FetchResponse));
    clone->body = NULL;
    if (resp->body && resp->body_len > 0) {
        clone->body = (char*)malloc(resp->body_len + 1);
        if (clone->body) {
            memcpy(clone->body, resp->body, resp->body_len);
            clone->body[resp->body_len] = '\0';
        }
    }
    return clone;
}
