#ifndef SSR_MODEL_H
#define SSR_MODEL_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define SSR_MAX_COMPONENTS      256
#define SSR_MAX_PROPS           64
#define SSR_HTML_BUFFER_SIZE    262144
#define SSR_STREAM_CHUNK_SIZE   4096
#define SSR_MAX_ROUTES          128
#define SSR_CACHE_ENTRIES       256
#define SSR_MAX_HYDRATION_NODES 1024

typedef enum {
    SSR_RENDER_STATIC,
    SSR_RENDER_STREAMING,
    SSR_RENDER_INCREMENTAL
} SSRRenderMode;

typedef enum {
    HYDRATION_IDLE,
    HYDRATION_STARTED,
    HYDRATION_IN_PROGRESS,
    HYDRATION_COMPLETE,
    HYDRATION_FAILED
} HydrationState;

typedef struct {
    char name[64];
    char value[4096];
} SSRProp;

typedef struct {
    char selector[256];
    char tag_name[64];
    int child_count;
    char child_selectors[64][256];
    char event_handlers[16][128];
    char attributes[32][2][128];
    bool is_hydrated;
    char server_html_hash[64];
} HydrationNode;

typedef struct {
    char component_name[128];
    SSRProp props[SSR_MAX_PROPS];
    int prop_count;
    char rendered_html[SSR_HTML_BUFFER_SIZE];
    size_t html_len;
    char css[32768];
    size_t css_len;
    HydrationNode hydration_nodes[SSR_MAX_HYDRATION_NODES];
    int hydration_node_count;
    bool is_interactive;
    int suspense_boundary_id;
    char slot_content[4][65536];
    int slot_count;
} SSRComponent;

typedef struct {
    char path[256];
    char component_name[128];
    bool is_dynamic;
    char params[16][2][128];
    int param_count;
    time_t revalidate_at;
    int revalidate_seconds;
    SSRComponent *component;
    char cached_html[SSR_HTML_BUFFER_SIZE];
    bool is_cached;
    time_t cached_at;
} SSRRouterEntry;

typedef struct {
    char head_html[65536];
    size_t head_len;
    char body_start[65536];
    size_t body_start_len;
    char body_end[65536];
    size_t body_end_len;
    char scripts[131072];
    size_t scripts_len;
    char inline_styles[65536];
    size_t inline_styles_len;
    char global_state_json[65536];
    size_t global_state_len;
} SSRDocument;

typedef struct {
    SSRRouterEntry routes[SSR_MAX_ROUTES];
    int route_count;
    SSRComponent components[SSR_MAX_COMPONENTS];
    int component_count;
    SSRDocument document;
    SSRRenderMode mode;
    bool hydrate_on_client;
    bool use_streaming;
    bool use_isr;
    char build_output_dir[MAX_PATH_LEN];
    char temp_cache_dir[MAX_PATH_LEN];
    int ssi_count;
    char ssi_entries[64][3][MAX_PATH_LEN];
} SSRConfig;

typedef struct {
    SSRConfig config;
    int rendered_pages;
    int hydrated_nodes;
    int cache_hits;
    int cache_misses;
    double avg_render_time_ms;
    size_t total_html_size;
} SSREngine;

void ssr_init(SSREngine *e);
void ssr_set_mode(SSREngine *e, SSRRenderMode mode);
int  ssr_register_component(SSREngine *e, const char *name);
void ssr_set_prop(SSRComponent *comp, const char *name, const char *value);
void ssr_set_slot(SSRComponent *comp, int index, const char *content);
int  ssr_register_route(SSREngine *e, const char *path, const char *component_name);
void ssr_set_revalidate(SSRRouterEntry *route, int seconds);
void ssr_render_to_string(SSREngine *e, SSRComponent *comp, char *out, size_t *len);
void ssr_render_page(SSREngine *e, const char *path, char *out, size_t *len);
void ssr_streaming_render(SSREngine *e, const char *path);
char* ssr_streaming_next_chunk(SSREngine *e);
bool ssr_streaming_has_more(SSREngine *e);
void ssr_hydrate(SSREngine *e, const char *html, size_t len);
void ssr_hydrate_node(HydrationNode *node);
void ssr_attach_event_handler(HydrationNode *node, const char *event, const char *handler);
int  ssr_static_generate(SSREngine *e, const char *output_dir);
int  ssr_isr_check(SSREngine *e, const char *path);
void ssr_isr_regenerate(SSREngine *e, SSRRouterEntry *route);
void ssr_inject_scripts(SSREngine *e, char *html, size_t *len);
void ssr_inject_state(SSREngine *e, char *html, size_t *len);
void ssr_print_stats(SSREngine *e);

#endif
