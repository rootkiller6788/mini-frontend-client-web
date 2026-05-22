#include "ssr_model.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

void ssr_init(SSREngine *e) {
    memset(e, 0, sizeof(*e));
    e->config.mode = SSR_RENDER_STATIC;
    e->config.hydrate_on_client = true;
    e->config.use_streaming = false;
    e->config.use_isr = false;
    strcpy(e->config.build_output_dir, "dist");
    strcpy(e->config.temp_cache_dir, ".ssr_cache");

    strcpy(e->document.head_html, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
        "<meta charset=\"UTF-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    e->document.head_len = strlen(e->document.head_html);
}

void ssr_set_mode(SSREngine *e, SSRRenderMode mode) {
    e->config.mode = mode;
    if (mode == SSR_RENDER_STREAMING) e->config.use_streaming = true;
    if (mode == SSR_RENDER_INCREMENTAL) e->config.use_isr = true;
}

int ssr_register_component(SSREngine *e, const char *name) {
    if (e->config.component_count >= SSR_MAX_COMPONENTS) return -1;
    SSRComponent *c = &e->config.components[e->config.component_count];
    memset(c, 0, sizeof(*c));
    strncpy(c->component_name, name, 127);
    e->config.component_count++;
    return e->config.component_count - 1;
}

void ssr_set_prop(SSRComponent *comp, const char *name, const char *value) {
    if (comp->prop_count >= SSR_MAX_PROPS) return;
    strncpy(comp->props[comp->prop_count].name, name, 63);
    strncpy(comp->props[comp->prop_count].value, value, 4095);
    comp->prop_count++;
}

void ssr_set_slot(SSRComponent *comp, int index, const char *content) {
    if (index < 0 || index > 3) return;
    strncpy(comp->slot_content[index], content, 65535);
    if (comp->slot_count <= index) comp->slot_count = index + 1;
}

int ssr_register_route(SSREngine *e, const char *path, const char *component_name) {
    if (e->config.route_count >= SSR_MAX_ROUTES) return -1;
    SSRRouterEntry *r = &e->config.routes[e->config.route_count];
    memset(r, 0, sizeof(*r));
    strncpy(r->path, path, 255);
    strncpy(r->component_name, component_name, 127);
    r->is_dynamic = strchr(path, ':') != NULL;
    r->is_cached = false;
    r->revalidate_at = 0;
    r->revalidate_seconds = 0;
    e->config.route_count++;
    return e->config.route_count - 1;
}

void ssr_set_revalidate(SSRRouterEntry *route, int seconds) {
    route->revalidate_seconds = seconds;
    route->revalidate_at = time(NULL) + seconds;
}

static int ssr_find_component(SSREngine *e, const char *name) {
    for (int i = 0; i < e->config.component_count; i++) {
        if (strcmp(e->config.components[i].component_name, name) == 0) return i;
    }
    return -1;
}

static int ssr_find_route(SSREngine *e, const char *path) {
    for (int i = 0; i < e->config.route_count; i++) {
        if (strcmp(e->config.routes[i].path, path) == 0) return i;
    }
    for (int i = 0; i < e->config.route_count; i++) {
        if (e->config.routes[i].is_dynamic) {
            const char *rp = e->config.routes[i].path;
            const char *up = path;
            bool match = true;
            while (*rp && *up && match) {
                if (*rp == ':') {
                    while (*rp && *rp != '/') rp++;
                    while (*up && *up != '/') up++;
                } else if (*rp == *up) {
                    rp++; up++;
                } else {
                    match = false;
                }
            }
            if (match && *rp == '\0' && *up == '\0') return i;
        }
    }
    return -1;
}

void ssr_render_to_string(SSREngine *e, SSRComponent *comp, char *out, size_t *len) {
    size_t offset = 0;
    int class_attr[64];
    memset(class_attr, 0, sizeof(class_attr));

    snprintf(out + offset, SSR_HTML_BUFFER_SIZE - offset,
        "<div data-component=\"%s\" data-ssr=\"true\">\n", comp->component_name);
    offset = strlen(out);

    for (int i = 0; i < comp->prop_count; i++) {
        snprintf(out + offset, SSR_HTML_BUFFER_SIZE - offset,
            "  <div data-prop=\"%s\">%s</div>\n",
            comp->props[i].name, comp->props[i].value);
        offset = strlen(out);
    }

    for (int i = 0; i < comp->slot_count; i++) {
        if (comp->slot_content[i][0] != '\0') {
            snprintf(out + offset, SSR_HTML_BUFFER_SIZE - offset,
                "  <div data-slot=\"%d\">%s</div>\n", i, comp->slot_content[i]);
            offset = strlen(out);
        }
    }

    if (comp->is_interactive) {
        for (int i = 0; i < comp->hydration_node_count && i < SSR_MAX_HYDRATION_NODES; i++) {
            HydrationNode *hn = &comp->hydration_nodes[i];
            snprintf(out + offset, SSR_HTML_BUFFER_SIZE - offset,
                "  <%s data-hydrate=\"%s\">", hn->tag_name, hn->selector);
            offset = strlen(out);
            for (int a = 0; a < 32; a++) {
                if (hn->attributes[a][0][0] != '\0') {
                    snprintf(out + offset, SSR_HTML_BUFFER_SIZE - offset,
                        " %s=\"%s\"", hn->attributes[a][0], hn->attributes[a][1]);
                    offset = strlen(out);
                }
            }
            snprintf(out + offset, SSR_HTML_BUFFER_SIZE - offset, "</%s>\n", hn->tag_name);
            offset = strlen(out);
        }
    }

    strcat(out + offset, "</div>\n");
    offset = strlen(out);

    if (comp->css_len > 0) {
        snprintf(out + offset, SSR_HTML_BUFFER_SIZE - offset,
            "<style data-component=\"%s\">%s</style>\n", comp->component_name, comp->css);
        offset = strlen(out);
    }

    if (len) *len = offset;
    strncpy(comp->rendered_html, out, SSR_HTML_BUFFER_SIZE - 1);
    comp->html_len = offset;
    e->rendered_pages++;
    e->total_html_size += offset;
    (void)class_attr;
}

void ssr_render_page(SSREngine *e, const char *path, char *out, size_t *len) {
    int ri = ssr_find_route(e, path);
    if (ri < 0) {
        snprintf(out, SSR_HTML_BUFFER_SIZE, "<h1>404 Not Found</h1>");
        if (len) *len = strlen(out);
        return;
    }

    SSRRouterEntry *route = &e->config.routes[ri];
    if (route->is_cached && route->revalidate_at > time(NULL)) {
        strncpy(out, route->cached_html, SSR_HTML_BUFFER_SIZE - 1);
        if (len) *len = strlen(out);
        e->cache_hits++;
        return;
    }
    e->cache_misses++;

    int ci = ssr_find_component(e, route->component_name);
    if (ci < 0) {
        snprintf(out, SSR_HTML_BUFFER_SIZE, "<h1>Component not found</h1>");
        if (len) *len = strlen(out);
        return;
    }

    char body[SSR_HTML_BUFFER_SIZE];
    size_t body_len;
    ssr_render_to_string(e, &e->config.components[ci], body, &body_len);

    size_t offset = 0;
    snprintf(out, SSR_HTML_BUFFER_SIZE, "%s", e->document.head_html);
    offset = strlen(out);

    snprintf(out + offset, SSR_HTML_BUFFER_SIZE - offset,
        "<title>%s</title>\n", route->path);
    offset = strlen(out);

    if (e->document.inline_styles_len > 0) {
        snprintf(out + offset, SSR_HTML_BUFFER_SIZE - offset,
            "<style>%s</style>\n", e->document.inline_styles);
        offset = strlen(out);
    }

    strcat(out + offset, "</head>\n<body>\n");
    offset = strlen(out);

    strncpy(out + offset, body, SSR_HTML_BUFFER_SIZE - offset - 1);
    offset = strlen(out);

    if (e->document.scripts_len > 0) {
        snprintf(out + offset, SSR_HTML_BUFFER_SIZE - offset,
            "%s\n", e->document.scripts);
        offset = strlen(out);
    }

    if (e->document.global_state_len > 0) {
        snprintf(out + offset, SSR_HTML_BUFFER_SIZE - offset,
            "<script>window.__INITIAL_STATE__=%s;</script>\n",
            e->document.global_state_json);
        offset = strlen(out);
    }

    strcat(out + offset, "</body>\n</html>");
    offset = strlen(out);

    if (e->config.use_isr && route->revalidate_seconds > 0) {
        strncpy(route->cached_html, out, SSR_HTML_BUFFER_SIZE - 1);
        route->is_cached = true;
        route->cached_at = time(NULL);
        ssr_set_revalidate(route, route->revalidate_seconds);
    }

    if (len) *len = offset;
}

void ssr_streaming_render(SSREngine *e, const char *path) {
    char chunk[SSR_HTML_BUFFER_SIZE];
    size_t clen;
    ssr_render_page(e, path, chunk, &clen);

    int header_end = 0;
    const char *body_start = strstr(chunk, "<body>");
    if (body_start) {
        header_end = (int)(body_start - chunk + 6);
        strncpy(e->document.body_start, chunk, header_end < (int)sizeof(e->document.body_start) - 1 ? header_end : (int)sizeof(e->document.body_start) - 1);
        e->document.body_start_len = strlen(e->document.body_start);

        const char *body_close = strstr(body_start, "</body>");
        if (body_close) {
            size_t body_len = body_close - body_start - 6;
            strncpy(e->document.body_end, body_close, sizeof(e->document.body_end) - 1);
            e->document.body_end_len = strlen(e->document.body_end);
            (void)body_len;
        }
    }
}

char* ssr_streaming_next_chunk(SSREngine *e) {
    static char chunk[SSR_STREAM_CHUNK_SIZE + 1];
    static int chunk_index = 0;
    static int total_chunks = 0;
    static char full_page[SSR_HTML_BUFFER_SIZE];

    if (chunk_index == 0) {
        int ri = ssr_find_route(e, "/");
        if (ri >= 0) {
            ssr_render_page(e, e->config.routes[ri].path, full_page, NULL);
            total_chunks = (int)(strlen(full_page) / SSR_STREAM_CHUNK_SIZE) + 1;
        }
    }

    if (chunk_index >= total_chunks) return NULL;
    size_t start = chunk_index * SSR_STREAM_CHUNK_SIZE;
    size_t copy = strlen(full_page + start);
    if (copy > SSR_STREAM_CHUNK_SIZE) copy = SSR_STREAM_CHUNK_SIZE;
    memcpy(chunk, full_page + start, copy);
    chunk[copy] = '\0';
    chunk_index++;
    return chunk;
}

bool ssr_streaming_has_more(SSREngine *e) {
    static int chunk_index = 0;
    (void)e;
    return chunk_index < 100;
}

void ssr_hydrate(SSREngine *e, const char *html, size_t len) {
    (void)len;
    e->hydrated_nodes = 0;
    const char *ptr = html;
    while ((ptr = strstr(ptr, "data-hydrate=")) != NULL) {
        const char *val = strchr(ptr, '"');
        if (!val) val = strchr(ptr, '\'');
        if (!val) break;

        char delim = *val;
        const char *ve = strchr(val + 1, delim);
        if (!ve) break;

        size_t slen = ve - val - 1;
        char selector[256];
        strncpy(selector, val + 1, slen < 255 ? slen : 255);
        selector[slen < 255 ? slen : 255] = '\0';

        for (int i = 0; i < e->config.component_count; i++) {
            SSRComponent *c = &e->config.components[i];
            for (int j = 0; j < c->hydration_node_count; j++) {
                if (strcmp(c->hydration_nodes[j].selector, selector) == 0) {
                    ssr_hydrate_node(&c->hydration_nodes[j]);
                }
            }
        }
        e->hydrated_nodes++;
        ptr = ve + 1;
    }
}

void ssr_hydrate_node(HydrationNode *node) {
    if (node->is_hydrated) return;
    node->is_hydrated = true;
    for (int i = 0; i < 16; i++) {
        if (node->event_handlers[i][0] != '\0') {
            strcat(node->server_html_hash, node->event_handlers[i]);
        }
    }
}

void ssr_attach_event_handler(HydrationNode *node, const char *event, const char *handler) {
    for (int i = 0; i < 16; i++) {
        if (node->event_handlers[i][0] == '\0') {
            snprintf(node->event_handlers[i], 127, "%s:%s", event, handler);
            return;
        }
    }
}

int ssr_static_generate(SSREngine *e, const char *output_dir) {
    int generated = 0;
    for (int i = 0; i < e->config.route_count; i++) {
        SSRRouterEntry *route = &e->config.routes[i];
        char html[SSR_HTML_BUFFER_SIZE];
        size_t html_len;
        ssr_render_page(e, route->path, html, &html_len);

        char filepath[MAX_PATH_LEN];
        if (strcmp(route->path, "/") == 0)
            snprintf(filepath, MAX_PATH_LEN, "%s/index.html", output_dir);
        else
            snprintf(filepath, MAX_PATH_LEN, "%s%s/index.html", output_dir, route->path);

        FILE *f = fopen(filepath, "wb");
        if (f) {
            fwrite(html, 1, html_len, f);
            fclose(f);
            generated++;
        }

        if (e->config.use_isr) {
            snprintf(filepath, MAX_PATH_LEN, "%s%s/.cache.html", e->config.temp_cache_dir, route->path);
            f = fopen(filepath, "wb");
            if (f) { fwrite(html, 1, html_len, f); fclose(f); }
        }
    }
    return generated;
}

int ssr_isr_check(SSREngine *e, const char *path) {
    int ri = ssr_find_route(e, path);
    if (ri < 0) return -1;
    SSRRouterEntry *route = &e->config.routes[ri];
    if (!route->revalidate_seconds) return 0;
    if (time(NULL) > route->revalidate_at) {
        ssr_isr_regenerate(e, route);
        return 1;
    }
    return 0;
}

void ssr_isr_regenerate(SSREngine *e, SSRRouterEntry *route) {
    char html[SSR_HTML_BUFFER_SIZE];
    size_t html_len;
    ssr_render_page(e, route->path, html, &html_len);
    strncpy(route->cached_html, html, SSR_HTML_BUFFER_SIZE - 1);
    route->is_cached = true;
    route->cached_at = time(NULL);
    ssr_set_revalidate(route, route->revalidate_seconds);
}

void ssr_inject_scripts(SSREngine *e, char *html, size_t *len) {
    const char *script_tag = "<script src=\"/bundle.js\" defer></script>";
    const char *head_close = "</head>";
    char *insert_pos = strstr(html, head_close);
    if (!insert_pos) return;

    size_t script_len = strlen(script_tag);
    size_t shift = strlen(insert_pos);
    memmove(insert_pos + script_len, insert_pos, shift + 1);
    memcpy(insert_pos, script_tag, script_len);
    if (len) *len = strlen(html);
    (void)e;
}

void ssr_inject_state(SSREngine *e, char *html, size_t *len) {
    if (!e->document.global_state_len) return;
    const char *body_close = "</body>";
    char *pos = strstr(html, body_close);
    if (!pos) return;

    char state_inject[65536 + 128];
    snprintf(state_inject, sizeof(state_inject),
        "<script>window.__INITIAL_STATE__=%s;</script>\n", e->document.global_state_json);
    size_t inj_len = strlen(state_inject);

    memmove(pos + inj_len, pos, strlen(pos) + 1);
    memcpy(pos, state_inject, inj_len);
    if (len) *len = strlen(html);
}

void ssr_print_stats(SSREngine *e) {
    printf("SSR Engine Stats:\n");
    printf("  Mode: %d\n", e->config.mode);
    printf("  Components: %d\n", e->config.component_count);
    printf("  Routes: %d\n", e->config.route_count);
    printf("  Pages rendered: %d\n", e->rendered_pages);
    printf("  Hydrated nodes: %d\n", e->hydrated_nodes);
    printf("  Cache hits: %d, misses: %d\n", e->cache_hits, e->cache_misses);
    printf("  Total HTML: %zu bytes\n", e->total_html_size);
}
