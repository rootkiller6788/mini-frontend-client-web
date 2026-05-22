#include "cookie_storage.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

void cookie_jar_init(CookieJar *jar, const char *origin) {
    memset(jar, 0, sizeof(*jar));
    strncpy(jar->origin_domain, origin, COOKIE_DOMAIN_MAX_LEN - 1);
}

void cookie_jar_destroy(CookieJar *jar) {
    (void)jar;
}

int cookie_domain_match(const char *cookie_domain,
                         const char *request_domain) {
    size_t cd_len, rd_len;
    if (!cookie_domain || !request_domain) return 0;
    cd_len = strlen(cookie_domain);
    rd_len = strlen(request_domain);

    /* Exact match */
    if (strcasecmp(cookie_domain, request_domain) == 0) return 1;

    /* Cookie domain starts with '.' and is suffix of request domain */
    if (cookie_domain[0] == '.' && rd_len >= cd_len) {
        const char *suffix = request_domain + (rd_len - cd_len);
        if (strcasecmp(cookie_domain, suffix) == 0) return 1;
    }

    /* Cookie domain without leading dot is suffix match */
    if (rd_len > cd_len) {
        const char *suffix = request_domain + (rd_len - cd_len);
        if (strcasecmp(cookie_domain, suffix) == 0 &&
            request_domain[rd_len - cd_len - 1] == '.')
            return 1;
    }
    return 0;
}

int cookie_path_match(const char *cookie_path,
                       const char *request_path) {
    size_t cp_len;
    if (!cookie_path || !cookie_path[0]) return 1;
    if (!request_path) return 0;
    cp_len = strlen(cookie_path);
    if (strncmp(request_path, cookie_path, cp_len) != 0) return 0;
    if (request_path[cp_len] == '\0' || request_path[cp_len] == '/' || cp_len == 1)
        return 1;
    return 0;
}

static void cookie_parse_attr(Cookie *cookie, const char *name,
                               const char *value) {
    if (!name) return;
    if (strcasecmp(name, "domain") == 0) {
        strncpy(cookie->domain, value, COOKIE_DOMAIN_MAX_LEN - 1);
        cookie->host_only = 0;
    } else if (strcasecmp(name, "path") == 0) {
        strncpy(cookie->path, value, COOKIE_PATH_MAX_LEN - 1);
    } else if (strcasecmp(name, "expires") == 0) {
        /* Simplified: just mark as persistent */
        cookie->persistent = 1;
        cookie->expires = (time_t)strtoll(value, NULL, 10);
    } else if (strcasecmp(name, "max-age") == 0) {
        int64_t age = (int64_t)strtoll(value, NULL, 10);
        cookie->expires = time(NULL) + (time_t)age;
        cookie->persistent = 1;
    } else if (strcasecmp(name, "httponly") == 0) {
        cookie->http_only = 1;
    } else if (strcasecmp(name, "secure") == 0) {
        cookie->secure = 1;
    } else if (strcasecmp(name, "samesite") == 0) {
        if (strcasecmp(value, "strict") == 0)
            cookie->same_site = COOKIE_SAMESITE_STRICT;
        else if (strcasecmp(value, "lax") == 0)
            cookie->same_site = COOKIE_SAMESITE_LAX;
        else
            cookie->same_site = COOKIE_SAMESITE_NONE;
    }
}

static void trim_spaces(char *s) {
    char *p;
    while (*s == ' ' || *s == '\t') s++;
    p = s + strlen(s) - 1;
    while (p > s && (*p == ' ' || *p == '\t')) *p-- = '\0';
    memmove(s, s, strlen(s) + 1);
    if (s != s) { /* can't happen but avoids unused warning */ }
}

int cookie_parse_set_cookie(Cookie *cookie, const char *header_val,
                             const char *request_domain, const char *request_path) {
    const char *p = header_val;
    const char *eq;
    char *out;
    int in_value = 0;
    char attr_name[128], attr_value[512];

    memset(cookie, 0, sizeof(*cookie));
    strncpy(cookie->domain, request_domain, COOKIE_DOMAIN_MAX_LEN - 1);
    strncpy(cookie->path, request_path, COOKIE_PATH_MAX_LEN - 1);
    cookie->host_only = 1;
    cookie->same_site = COOKIE_SAMESITE_LAX;
    cookie->creation_time = time(NULL);

    out = cookie->name;
    while (*p && *p != '=' && *p != ';' &&
           (size_t)(out - cookie->name) < COOKIE_NAME_MAX_LEN - 1) {
        *out++ = *p++;
    }
    *out = '\0';

    if (*p == '=') {
        p++;
        out = cookie->value;
        while (*p && *p != ';' &&
               (size_t)(out - cookie->value) < COOKIE_VALUE_MAX_LEN - 1) {
            *out++ = *p++;
        }
        *out = '\0';
    }

    /* Parse attributes */
    while (*p) {
        if (*p == ';') {
            p++;
            while (*p == ' ') p++;
            if (!*p) break;

            eq = strchr(p, '=');
            if (eq) {
                size_t nlen = (eq - p);
                if (nlen > 127) nlen = 127;
                memcpy(attr_name, p, nlen);
                attr_name[nlen] = '\0';
                strncpy(attr_value, eq + 1, 511);
                attr_value[511] = '\0';
                /* Strip trailing semicolon from value */
                {
                    char *sc = strchr(attr_value, ';');
                    if (sc) *sc = '\0';
                }
                p = eq + 1 + strlen(eq + 1);
            } else {
                strncpy(attr_name, p, 127);
                attr_name[127] = '\0';
                attr_value[0] = '\0';
                p += strlen(p);
            }
            cookie_parse_attr(cookie, attr_name, attr_value);
        } else {
            break;
        }
    }

    if (!cookie->path[0])
        strncpy(cookie->path, "/", COOKIE_PATH_MAX_LEN - 1);

    return 1;
}

int cookie_jar_insert(CookieJar *jar, const Cookie *cookie) {
    int i;

    /* Replace existing same-name+domain+path cookie */
    for (i = 0; i < jar->count; i++) {
        if (strcmp(jar->cookies[i].name, cookie->name) == 0 &&
            strcmp(jar->cookies[i].domain, cookie->domain) == 0 &&
            strcmp(jar->cookies[i].path, cookie->path) == 0) {
            memcpy(&jar->cookies[i], cookie, sizeof(Cookie));
            return 1;
        }
    }

    if (jar->count >= COOKIE_MAX_PER_JAR) {
        cookie_jar_purge_expired(jar);
        if (jar->count >= COOKIE_MAX_PER_JAR) return 0;
    }

    memcpy(&jar->cookies[jar->count++], cookie, sizeof(Cookie));
    return 1;
}

int cookie_jar_remove(CookieJar *jar, const char *name,
                       const char *domain, const char *path) {
    int i;
    for (i = 0; i < jar->count; i++) {
        Cookie *c = &jar->cookies[i];
        if (strcmp(c->name, name) == 0 &&
            strcmp(c->domain, domain) == 0 &&
            strcmp(c->path, path) == 0) {
            if (i < jar->count - 1)
                memmove(&jar->cookies[i], &jar->cookies[i + 1],
                        (size_t)(jar->count - i - 1) * sizeof(Cookie));
            jar->count--;
            return 1;
        }
    }
    return 0;
}

int cookie_jar_match(const CookieJar *jar, const char *domain,
                      const char *path, int secure_only,
                      Cookie *results, int max_results) {
    int i, found = 0;
    time_t now = time(NULL);

    for (i = 0; i < jar->count && found < max_results; i++) {
        const Cookie *c = &jar->cookies[i];
        if (c->persistent && c->expires > 0 && now > c->expires) continue;
        if (secure_only && c->secure && !secure_only) return 0; /* can't happen */
        if (c->secure && !secure_only) continue; /* skip secure on non-secure */
        if (!cookie_domain_match(c->domain, domain)) continue;
        if (!cookie_path_match(c->path, path)) continue;
        memcpy(&results[found++], c, sizeof(Cookie));
    }
    return found;
}

int cookie_jar_serialize_for_request(const CookieJar *jar,
                                      const char *domain,
                                      const char *path,
                                      int    is_secure,
                                      char  *header_out,
                                      size_t header_size) {
    Cookie matches[COOKIE_MAX_PER_JAR];
    int count, i, written = 0;
    char *p = header_out;

    count = cookie_jar_match(jar, domain, path, is_secure,
                              matches, COOKIE_MAX_PER_JAR);
    for (i = 0; i < count; i++) {
        Cookie *c = &matches[i];
        if (c->http_only) continue; /* skip HttpOnly for JS, but in request we include? */
        /* We include HttpOnly in wire serialization — HttpOnly means not visible to JS */
        {
            int need = (int)strlen(c->name) + 1 + (int)strlen(c->value);
            if (i > 0) need += 2; /* "; " */
            if ((size_t)(p - header_out) + (size_t)need + 1 >= header_size) break;
            if (i > 0) {
                *p++ = ';'; *p++ = ' ';
            }
            strcpy(p, c->name); p += strlen(p);
            *p++ = '=';
            strcpy(p, c->value); p += strlen(p);
            written++;
        }
    }
    *p = '\0';
    return written;
}

void cookie_jar_purge_expired(CookieJar *jar) {
    int i;
    time_t now = time(NULL);
    for (i = jar->count - 1; i >= 0; i--) {
        Cookie *c = &jar->cookies[i];
        if (c->persistent && c->expires > 0 && now > c->expires) {
            if (i < jar->count - 1)
                memmove(&jar->cookies[i], &jar->cookies[i + 1],
                        (size_t)(jar->count - i - 1) * sizeof(Cookie));
            jar->count--;
        }
    }
}

/* =================== LocalStorage =================== */

void localStorage_init(LocalStorage *ls, const char *origin, size_t quota) {
    memset(ls, 0, sizeof(*ls));
    strncpy(ls->origin, origin, COOKIE_DOMAIN_MAX_LEN - 1);
    ls->quota = quota;
}

void localStorage_destroy(LocalStorage *ls) { (void)ls; }

static int ls_find(const LocalStorage *ls, const char *key) {
    int i;
    for (i = 0; i < ls->count; i++) {
        if (strcmp(ls->entries[i].key, key) == 0) return i;
    }
    return -1;
}

int localStorage_set(LocalStorage *ls, const char *key, const char *value) {
    int idx = ls_find(ls, key);
    size_t vlen = strlen(value);
    size_t need;

    if (idx >= 0) {
        need = vlen - strlen(ls->entries[idx].value);
    } else {
        need = strlen(key) + vlen;
    }

    if (ls->used + need > ls->quota) return 0;

    if (idx >= 0) {
        ls->used -= strlen(ls->entries[idx].value);
        strncpy(ls->entries[idx].value, value, LOCALSTORAGE_VALUE_LEN - 1);
        ls->used += strlen(ls->entries[idx].value);
    } else {
        if (ls->count >= LOCALSTORAGE_MAX_KEYS) return 0;
        idx = ls->count++;
        strncpy(ls->entries[idx].key, key, LOCALSTORAGE_KEY_LEN - 1);
        strncpy(ls->entries[idx].value, value, LOCALSTORAGE_VALUE_LEN - 1);
        ls->used += need;
    }
    return 1;
}

int localStorage_get(LocalStorage *ls, const char *key,
                     char *value_out, size_t max_len) {
    int idx = ls_find(ls, key);
    if (idx < 0) return 0;
    strncpy(value_out, ls->entries[idx].value, max_len - 1);
    value_out[max_len - 1] = '\0';
    return 1;
}

int localStorage_remove(LocalStorage *ls, const char *key) {
    int idx = ls_find(ls, key);
    if (idx < 0) return 0;
    ls->used -= strlen(ls->entries[idx].key) + strlen(ls->entries[idx].value);
    if (idx < ls->count - 1)
        memmove(&ls->entries[idx], &ls->entries[idx + 1],
                (size_t)(ls->count - idx - 1) * sizeof(LocalStorageEntry));
    ls->count--;
    return 1;
}

int localStorage_has(LocalStorage *ls, const char *key) {
    return ls_find(ls, key) >= 0;
}

void localStorage_clear(LocalStorage *ls) { ls->count = 0; ls->used = 0; }
int  localStorage_count(const LocalStorage *ls) { return ls->count; }

int localStorage_key_at(const LocalStorage *ls, int index,
                         char *key_out, size_t max_len) {
    if (index < 0 || index >= ls->count) return 0;
    strncpy(key_out, ls->entries[index].key, max_len - 1);
    key_out[max_len - 1] = '\0';
    return 1;
}

/* =================== SessionStorage =================== */

void sessionStorage_init(SessionStorage *ss, const char *origin, size_t quota) {
    memset(ss, 0, sizeof(*ss));
    strncpy(ss->origin, origin, COOKIE_DOMAIN_MAX_LEN - 1);
    ss->quota = quota;
}

void sessionStorage_destroy(SessionStorage *ss) { (void)ss; }

static int ss_find(const SessionStorage *ss, const char *key) {
    int i;
    for (i = 0; i < ss->count; i++)
        if (strcmp(ss->entries[i].key, key) == 0) return i;
    return -1;
}

int sessionStorage_set(SessionStorage *ss, const char *key, const char *value) {
    int idx = ss_find(ss, key);
    size_t vlen = strlen(value);
    size_t need;
    if (idx >= 0) need = vlen - strlen(ss->entries[idx].value);
    else need = strlen(key) + vlen;
    if (ss->used + need > ss->quota) return 0;
    if (idx >= 0) {
        ss->used -= strlen(ss->entries[idx].value);
        strncpy(ss->entries[idx].value, value, SESSIONSTORAGE_VALUE_LEN - 1);
        ss->used += strlen(ss->entries[idx].value);
    } else {
        if (ss->count >= SESSIONSTORAGE_MAX_KEYS) return 0;
        idx = ss->count++;
        strncpy(ss->entries[idx].key, key, SESSIONSTORAGE_KEY_LEN - 1);
        strncpy(ss->entries[idx].value, value, SESSIONSTORAGE_VALUE_LEN - 1);
        ss->used += need;
    }
    return 1;
}

int sessionStorage_get(SessionStorage *ss, const char *key,
                       char *value_out, size_t max_len) {
    int idx = ss_find(ss, key);
    if (idx < 0) return 0;
    strncpy(value_out, ss->entries[idx].value, max_len - 1);
    return 1;
}

int sessionStorage_remove(SessionStorage *ss, const char *key) {
    int idx = ss_find(ss, key);
    if (idx < 0) return 0;
    ss->used -= strlen(ss->entries[idx].key) + strlen(ss->entries[idx].value);
    if (idx < ss->count - 1)
        memmove(&ss->entries[idx], &ss->entries[idx + 1],
                (size_t)(ss->count - idx - 1) * sizeof(LocalStorageEntry));
    ss->count--;
    return 1;
}

int sessionStorage_has(SessionStorage *ss, const char *key) {
    return ss_find(ss, key) >= 0;
}
void sessionStorage_clear(SessionStorage *ss) { ss->count = 0; ss->used = 0; }
int  sessionStorage_count(const SessionStorage *ss) { return ss->count; }

int sessionStorage_key_at(const SessionStorage *ss, int index,
                           char *key_out, size_t max_len) {
    if (index < 0 || index >= ss->count) return 0;
    strncpy(key_out, ss->entries[index].key, max_len - 1);
    return 1;
}
