#ifndef COOKIE_STORAGE_H
#define COOKIE_STORAGE_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#define COOKIE_MAX_PER_JAR     512
#define COOKIE_NAME_MAX_LEN    256
#define COOKIE_VALUE_MAX_LEN   4096
#define COOKIE_DOMAIN_MAX_LEN  256
#define COOKIE_PATH_MAX_LEN    256
#define COOKIE_MAX_ATTRIBUTES  8

#define LOCALSTORAGE_MAX_KEYS  1024
#define LOCALSTORAGE_KEY_LEN   256
#define LOCALSTORAGE_VALUE_LEN 16384

#define SESSIONSTORAGE_MAX_KEYS  512
#define SESSIONSTORAGE_KEY_LEN   256
#define SESSIONSTORAGE_VALUE_LEN 16384
#define SESSIONSTORAGE_MAX_TABS  32

typedef enum {
    COOKIE_SAMESITE_NONE   = 0,
    COOKIE_SAMESITE_LAX    = 1,
    COOKIE_SAMESITE_STRICT = 2
} CookieSameSite;

typedef struct {
    char name[COOKIE_NAME_MAX_LEN];
    char value[COOKIE_VALUE_MAX_LEN];
    char domain[COOKIE_DOMAIN_MAX_LEN];
    char path[COOKIE_PATH_MAX_LEN];

    time_t expires;
    int    persistent;
    int    http_only;
    int    secure;
    CookieSameSite same_site;

    int    host_only;
    time_t creation_time;
    time_t last_access;
} Cookie;

typedef struct {
    Cookie cookies[COOKIE_MAX_PER_JAR];
    int    count;
    char   origin_domain[COOKIE_DOMAIN_MAX_LEN];
} CookieJar;

/* LocalStorage key-value pair (persistent per-origin) */
typedef struct {
    char key[LOCALSTORAGE_KEY_LEN];
    char value[LOCALSTORAGE_VALUE_LEN];
} LocalStorageEntry;

typedef struct {
    LocalStorageEntry entries[LOCALSTORAGE_MAX_KEYS];
    int count;
    char origin[COOKIE_DOMAIN_MAX_LEN];
    size_t quota;
    size_t used;
} LocalStorage;

/* SessionStorage (per-tab, same layout as LocalStorage) */
typedef struct {
    LocalStorageEntry entries[SESSIONSTORAGE_MAX_KEYS];
    int count;
    char origin[COOKIE_DOMAIN_MAX_LEN];
    size_t quota;
    size_t used;
} SessionStorage;

void  cookie_jar_init(CookieJar *jar, const char *origin);
void  cookie_jar_destroy(CookieJar *jar);

int   cookie_parse_set_cookie(Cookie *cookie, const char *header_val,
                              const char *request_domain, const char *request_path);

int   cookie_jar_insert(CookieJar *jar, const Cookie *cookie);
int   cookie_jar_remove(CookieJar *jar, const char *name,
                        const char *domain, const char *path);

int   cookie_jar_match(const CookieJar *jar, const char *domain,
                       const char *path, int secure_only,
                       Cookie *results, int max_results);

int   cookie_jar_serialize_for_request(const CookieJar *jar,
                                        const char *domain,
                                        const char *path,
                                        int    is_secure,
                                        char  *header_out,
                                        size_t header_size);

void  cookie_jar_purge_expired(CookieJar *jar);

int   cookie_domain_match(const char *cookie_domain,
                          const char *request_domain);
int   cookie_path_match(const char *cookie_path,
                        const char *request_path);

/* --- LocalStorage --- */
void  localStorage_init(LocalStorage *ls, const char *origin,
                        size_t quota);
void  localStorage_destroy(LocalStorage *ls);

int   localStorage_set(LocalStorage *ls, const char *key,
                       const char *value);
int   localStorage_get(LocalStorage *ls, const char *key,
                       char *value_out, size_t max_len);
int   localStorage_remove(LocalStorage *ls, const char *key);
int   localStorage_has(LocalStorage *ls, const char *key);
void  localStorage_clear(LocalStorage *ls);
int   localStorage_count(const LocalStorage *ls);
int   localStorage_key_at(const LocalStorage *ls, int index,
                          char *key_out, size_t max_len);

/* --- SessionStorage (per-tab) --- */
void  sessionStorage_init(SessionStorage *ss, const char *origin,
                          size_t quota);
void  sessionStorage_destroy(SessionStorage *ss);
int   sessionStorage_set(SessionStorage *ss, const char *key,
                         const char *value);
int   sessionStorage_get(SessionStorage *ss, const char *key,
                         char *value_out, size_t max_len);
int   sessionStorage_remove(SessionStorage *ss, const char *key);
int   sessionStorage_has(SessionStorage *ss, const char *key);
void  sessionStorage_clear(SessionStorage *ss);
int   sessionStorage_count(const SessionStorage *ss);
int   sessionStorage_key_at(const SessionStorage *ss, int index,
                            char *key_out, size_t max_len);

#endif
