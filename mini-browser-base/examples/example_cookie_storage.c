#include "cookie_storage.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    CookieJar jar;
    Cookie cookie;
    char header[8192];
    int count;

    printf("=== Cookie Jar Demo ===\n\n");
    cookie_jar_init(&jar, "example.com");

    /* Parse Set-Cookie strings */
    cookie_parse_set_cookie(&cookie,
        "session=abc123; Domain=example.com; Path=/; HttpOnly; Secure; SameSite=Lax",
        "example.com", "/");
    cookie_jar_insert(&jar, &cookie);
    printf("Inserted: %s=%s (HttpOnly=%d, Secure=%d, SameSite=%d)\n",
           cookie.name, cookie.value, cookie.http_only, cookie.secure,
           cookie.same_site);

    cookie_parse_set_cookie(&cookie,
        "theme=dark; Domain=.example.com; Path=/; Max-Age=3600; SameSite=Strict",
        "example.com", "/");
    cookie_jar_insert(&jar, &cookie);
    printf("Inserted: %s=%s (persistent=%d, expires=%lld)\n",
           cookie.name, cookie.value, cookie.persistent,
           (long long)cookie.expires);

    cookie_parse_set_cookie(&cookie,
        "pref=en; Domain=www.example.com; Path=/app",
        "www.example.com", "/app");
    cookie_jar_insert(&jar, &cookie);
    printf("Inserted: %s=%s (domain=%s, path=%s)\n",
           cookie.name, cookie.value, cookie.domain, cookie.path);

    /* Domain/path matching */
    printf("\n--- Matching tests ---\n");
    printf("example.com matches .example.com? %d\n",
           cookie_domain_match(".example.com", "example.com"));
    printf("www.example.com matches .example.com? %d\n",
           cookie_domain_match(".example.com", "www.example.com"));
    printf("/ matches /? %d\n", cookie_path_match("/", "/"));
    printf("/app matches /app/page? %d\n", cookie_path_match("/app", "/app/page"));
    printf("/app matches /other? %d\n", cookie_path_match("/app", "/other"));

    /* Serialize for request */
    count = cookie_jar_serialize_for_request(&jar,
        "www.example.com", "/app/page", 1, header, sizeof(header));
    printf("\n--- Cookie header for https://www.example.com/app/page ---\n");
    printf("Cookie: %s\n", header);
    printf("%d cookies sent (HttpOnly=%d included)\n", count, 1);

    /* LocalStorage */
    printf("\n=== LocalStorage Demo ===\n");
    {
        LocalStorage ls;
        char val[256];
        localStorage_init(&ls, "https://example.com", 5 * 1024 * 1024);

        localStorage_set(&ls, "theme", "dark");
        localStorage_set(&ls, "lang", "en-US");
        localStorage_set(&ls, "cart", "{\"items\":3}");

        printf("Count: %d\n", localStorage_count(&ls));
        localStorage_get(&ls, "theme", val, sizeof(val));
        printf("theme = %s\n", val);
        localStorage_get(&ls, "lang", val, sizeof(val));
        printf("lang = %s\n", val);

        printf("has('cart') = %d\n", localStorage_has(&ls, "cart"));
        localStorage_remove(&ls, "cart");
        printf("has('cart') after remove = %d\n", localStorage_has(&ls, "cart"));

        {
            char k[256];
            int i;
            printf("All keys:\n");
            for (i = 0; i < localStorage_count(&ls); i++) {
                localStorage_key_at(&ls, i, k, sizeof(k));
                printf("  [%d] %s\n", i, k);
            }
        }

        localStorage_clear(&ls);
        printf("After clear: count=%d\n", localStorage_count(&ls));
    }

    /* SessionStorage */
    printf("\n=== SessionStorage Demo ===\n");
    {
        SessionStorage ss;
        char val[256];
        sessionStorage_init(&ss, "https://example.com", 3 * 1024 * 1024);

        sessionStorage_set(&ss, "tab-id", "42A");
        sessionStorage_set(&ss, "scroll-pos", "1500");

        sessionStorage_get(&ss, "tab-id", val, sizeof(val));
        printf("tab-id = %s\n", val);
        sessionStorage_get(&ss, "scroll-pos", val, sizeof(val));
        printf("scroll-pos = %s\n", val);
        printf("Count: %d\n", sessionStorage_count(&ss));
    }

    cookie_jar_destroy(&jar);
    return 0;
}
