#ifndef FETCH_MODEL_H
#define FETCH_MODEL_H

#include <stdint.h>
#include <stddef.h>

#define FETCH_MAX_HEADERS       64
#define FETCH_HEADER_NAME_LEN   128
#define FETCH_HEADER_VALUE_LEN  1024
#define FETCH_URL_MAX_LEN       2048
#define FETCH_METHOD_MAX_LEN    16
#define FETCH_BODY_MAX_LEN      262144
#define FETCH_STATUS_TEXT_LEN   128
#define FETCH_MAX_INTERCEPTORS  16

typedef enum {
    FETCH_MODE_SAME_ORIGIN  = 0,
    FETCH_MODE_CORS         = 1,
    FETCH_MODE_NO_CORS      = 2,
    FETCH_MODE_NAVIGATE     = 3
} FetchMode;

typedef enum {
    FETCH_CREDENTIALS_OMIT        = 0,
    FETCH_CREDENTIALS_SAME_ORIGIN = 1,
    FETCH_CREDENTIALS_INCLUDE     = 2
} FetchCredentials;

typedef enum {
    FETCH_REDIRECT_FOLLOW = 0,
    FETCH_REDIRECT_ERROR  = 1,
    FETCH_REDIRECT_MANUAL = 2
} FetchRedirect;

typedef struct {
    char name[FETCH_HEADER_NAME_LEN];
    char value[FETCH_HEADER_VALUE_LEN];
} FetchHeader;

typedef struct {
    char    method[FETCH_METHOD_MAX_LEN];
    char    url[FETCH_URL_MAX_LEN];
    FetchHeader headers[FETCH_MAX_HEADERS];
    int     header_count;
    char   *body;
    size_t  body_len;
    FetchMode       mode;
    FetchCredentials credentials;
    FetchRedirect   redirect;
    char   *referrer;
    int     keepalive;
    int     integrity_valid;
    void   *signal;   /* AbortSignal ptr */
} FetchRequest;

typedef struct {
    int     status;
    char    status_text[FETCH_STATUS_TEXT_LEN];
    FetchHeader headers[FETCH_MAX_HEADERS];
    int     header_count;
    char   *body;
    size_t  body_len;
    int     ok;
    int     redirected;
    char    url[FETCH_URL_MAX_LEN];
    char    type[FETCH_METHOD_MAX_LEN];  /* basic, cors, opaque, error */
} FetchResponse;

/* --- AbortController --- */
typedef struct {
    int     aborted;
    void   *callbacks[FETCH_MAX_HEADERS];
    int     callback_count;
} AbortSignal;

typedef struct {
    AbortSignal signal;
} AbortController;

void abort_controller_init(AbortController *ctrl);
void abort_controller_abort(AbortController *ctrl);
int  abort_signal_aborted(const AbortSignal *signal);

/* --- CORS simulation --- */
typedef enum {
    CORS_ALLOWED           = 0,
    CORS_BLOCKED           = 1,
    CORS_PREFLIGHT_REQUIRED = 2
} CorsResult;

typedef struct {
    char    origin[FETCH_URL_MAX_LEN];
    char    allowed_origins[FETCH_URL_MAX_LEN];
    int     allow_credentials;
} CorsConfig;

void  cors_config_init(CorsConfig *cfg, const char *origin,
                       const char *allowed_origins, int allow_credentials);

CorsResult cors_check_simple(const FetchRequest *req, const CorsConfig *cfg);
CorsResult cors_check_preflight(const FetchRequest *req,
                                const CorsConfig *cfg,
                                const char *req_method,
                                const char *req_headers);

/* --- Request Interceptor --- */
typedef FetchResponse *(*FetchInterceptor)(const FetchRequest *req,
                                            void *user_data);

typedef struct {
    FetchInterceptor callback;
    void            *user_data;
    int              active;
} FetchInterceptorEntry;

typedef struct {
    FetchInterceptorEntry entries[FETCH_MAX_INTERCEPTORS];
    int                    count;
} FetchInterceptorChain;

void fetch_interceptor_chain_init(FetchInterceptorChain *chain);
int  fetch_interceptor_chain_add(FetchInterceptorChain *chain,
                                  FetchInterceptor cb, void *user_data);
int  fetch_interceptor_chain_remove(FetchInterceptorChain *chain,
                                     FetchInterceptor cb);
FetchResponse *fetch_interceptor_chain_run(FetchInterceptorChain *chain,
                                            const FetchRequest *req);

/* --- Request / Response helpers --- */
void fetch_request_init(FetchRequest *req, const char *method,
                        const char *url);
void fetch_request_destroy(FetchRequest *req);
int  fetch_request_set_header(FetchRequest *req, const char *name,
                              const char *value);
int  fetch_request_get_header(const FetchRequest *req, const char *name,
                              char *value_out, size_t max_len);
void fetch_request_set_body(FetchRequest *req, const char *body,
                            size_t len);

void fetch_response_init(FetchResponse *resp, int status,
                         const char *status_text);
void fetch_response_destroy(FetchResponse *resp);
int  fetch_response_set_header(FetchResponse *resp, const char *name,
                               const char *value);
int  fetch_response_get_header(const FetchResponse *resp, const char *name,
                               char *value_out, size_t max_len);
void fetch_response_set_body(FetchResponse *resp, const char *body,
                             size_t len);

FetchResponse *fetch_response_clone(const FetchResponse *resp);

#endif
