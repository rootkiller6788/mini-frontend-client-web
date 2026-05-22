#include "ssr_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void define_components(SSREngine *engine) {
    int home_id = ssr_register_component(engine, "HomePage");
    int blog_id = ssr_register_component(engine, "BlogPage");
    int product_id = ssr_register_component(engine, "ProductPage");
    int about_id = ssr_register_component(engine, "AboutPage");
    int profile_id = ssr_register_component(engine, "UserProfile");

    SSRComponent *home = &engine->config.components[home_id];
    ssr_set_prop(home, "title", "Welcome to Mini SSR");
    ssr_set_prop(home, "description", "A lightweight SSR implementation in C99");
    ssr_set_prop(home, "version", "1.0.0");
    ssr_set_prop(home, "features", "SSR,SSG,ISR,Streaming,Hydration");
    home->is_interactive = true;
    home->hydration_nodes[0].tag_name[0] = 'd';
    home->hydration_nodes[0].tag_name[1] = 'i';
    home->hydration_nodes[0].tag_name[2] = 'v';
    home->hydration_nodes[0].tag_name[3] = '\0';
    strcpy(home->hydration_nodes[0].selector, "home-counter");
    home->hydration_node_count = 1;

    SSRComponent *blog = &engine->config.components[blog_id];
    ssr_set_prop(blog, "title", "Blog");
    ssr_set_prop(blog, "posts", "[{\"id\":1,\"title\":\"Hello World\"}]");
    blog->is_interactive = false;

    SSRComponent *product = &engine->config.components[product_id];
    ssr_set_prop(product, "title", "Products");
    ssr_set_prop(product, "sku", "PROD-001");
    product->is_interactive = true;
    strcpy(product->hydration_nodes[0].tag_name, "button");
    strcpy(product->hydration_nodes[0].selector, "add-to-cart");
    strcpy(product->hydration_nodes[0].attributes[0][0], "class");
    strcpy(product->hydration_nodes[0].attributes[0][1], "btn-primary");
    ssr_attach_event_handler(&product->hydration_nodes[0], "click", "handleAddToCart");
    product->hydration_node_count = 1;

    SSRComponent *about = &engine->config.components[about_id];
    ssr_set_prop(about, "title", "About Us");
    ssr_set_prop(about, "mission", "Build fast web experiences");

    SSRComponent *profile = &engine->config.components[profile_id];
    ssr_set_prop(profile, "title", "User Profile");
    ssr_set_prop(profile, "username", "john_doe");
    profile->is_interactive = true;

    printf("Components registered: %d\n", engine->config.component_count);
    (void)profile_id;
    (void)about_id;
}

static void define_routes(SSREngine *engine) {
    ssr_register_route(engine, "/", "HomePage");
    ssr_register_route(engine, "/blog", "BlogPage");
    ssr_register_route(engine, "/blog/:slug", "BlogPage");
    ssr_register_route(engine, "/products", "ProductPage");
    ssr_register_route(engine, "/products/:id", "ProductPage");
    ssr_register_route(engine, "/about", "AboutPage");
    ssr_register_route(engine, "/profile/:username", "UserProfile");

    engine->config.routes[0].revalidate_seconds = 60;
    ssr_set_revalidate(&engine->config.routes[0], 60);
    engine->config.routes[2].is_dynamic = true;
    engine->config.routes[4].is_dynamic = true;
    engine->config.routes[6].is_dynamic = true;

    printf("Routes registered: %d\n", engine->config.route_count);
}

static void demo_static_render(SSREngine *engine) {
    printf("\n=== Demo 1: Static Render (renderToString) ===\n\n");

    for (int i = 0; i < engine->config.component_count && i < 3; i++) {
        SSRComponent *c = &engine->config.components[i];
        char html[SSR_HTML_BUFFER_SIZE];
        size_t html_len;
        ssr_render_to_string(engine, c, html, &html_len);
        printf("--- %s ---\n%s\n(size: %zu bytes)\n\n", c->component_name, html, html_len);
    }
}

static void demo_page_render(SSREngine *engine) {
    printf("\n=== Demo 2: Full Page Render ===\n\n");

    const char *pages[] = {"/", "/blog", "/products", "/about"};
    for (int i = 0; i < 4; i++) {
        char html[SSR_HTML_BUFFER_SIZE];
        size_t html_len;
        ssr_render_page(engine, pages[i], html, &html_len);
        printf("--- %s ---\n", pages[i]);
        printf("%.500s\n", html);
        if (html_len > 500) printf("...(truncated, %zu total bytes)\n", html_len);
        printf("\n");
    }
}

static void demo_hydration(SSREngine *engine) {
    printf("\n=== Demo 3: Hydration ===\n\n");

    const char *server_html =
        "<!DOCTYPE html><html><head></head><body>\n"
        "<div data-component=\"ProductPage\" data-ssr=\"true\">\n"
        "  <button data-hydrate=\"add-to-cart\" class=\"btn-primary\">Add to Cart</button>\n"
        "  <div data-hydrate=\"not-found\">Missing</div>\n"
        "</div>\n"
        "</body></html>";

    printf("Server-rendered HTML received.\n");
    printf("Hydrating...\n");

    ssr_hydrate(engine, server_html, strlen(server_html));

    printf("Hydrated nodes: %d\n", engine->hydrated_nodes);

    SSRComponent *prod = &engine->config.components[2];
    for (int i = 0; i < prod->hydration_node_count; i++) {
        HydrationNode *hn = &prod->hydration_nodes[i];
        printf("  Node '%s': hydrated=%d\n", hn->selector, hn->is_hydrated);
        for (int j = 0; j < 16; j++) {
            if (hn->event_handlers[j][0] != '\0')
                printf("    Event: %s\n", hn->event_handlers[j]);
        }
    }
}

static void demo_isr(SSREngine *engine) {
    printf("\n=== Demo 4: Incremental Static Regeneration (ISR) ===\n\n");

    ssr_set_mode(engine, SSR_RENDER_INCREMENTAL);

    SSRRouterEntry *home = &engine->config.routes[0];
    home->revalidate_seconds = 10;
    ssr_set_revalidate(home, 10);

    printf("Home page ISR configured: revalidate every %d seconds\n", home->revalidate_seconds);
    printf("Current time: %lld\n", (long long)time(NULL));
    printf("Revalidate at: %lld\n\n", (long long)home->revalidate_at);

    printf("First request (cache miss):\n");
    char html[SSR_HTML_BUFFER_SIZE];
    size_t html_len;
    ssr_render_page(engine, "/", html, &html_len);
    printf("  Cache: hit=%d miss=%d\n", engine->cache_hits, engine->cache_misses);
    printf("  HTML size: %zu bytes\n", html_len);

    printf("\nSecond request (cache hit, still fresh):\n");
    ssr_render_page(engine, "/", html, &html_len);
    printf("  Cache: hit=%d miss=%d\n", engine->cache_hits, engine->cache_misses);

    printf("\nSimulating stale cache:\n");
    home->revalidate_at = time(NULL) - 1;
    printf("  Set revalidate_at to past time...\n");

    int result = ssr_isr_check(engine, "/");
    printf("  ISR check result: %d (1=regenerated)\n", result);

    printf("\nRequest after regeneration:\n");
    home->is_cached = false;
    ssr_render_page(engine, "/", html, &html_len);
    printf("  Cache: hit=%d miss=%d\n", engine->cache_hits, engine->cache_misses);
}

static void demo_static_generation(SSREngine *engine) {
    printf("\n=== Demo 5: Static Site Generation (SSG) ===\n\n");

    printf("Generating static HTML for all routes...\n");
    int generated = ssr_static_generate(engine, "ssg_output");
    printf("Pages generated: %d\n", generated);

    const char *expected[] = {
        "ssg_output/index.html",
        "ssg_output/blog/index.html",
        "ssg_output/products/index.html",
        "ssg_output/about/index.html",
    };

    for (int i = 0; i < 4; i++) {
        FILE *f = fopen(expected[i], "rb");
        if (f) {
            long size;
            fseek(f, 0, SEEK_END);
            size = ftell(f);
            fclose(f);
            printf("  %s: %ld bytes\n", expected[i], size);
        } else {
            printf("  %s: NOT CREATED\n", expected[i]);
        }
    }
}

static void demo_script_injection(SSREngine *engine) {
    printf("\n=== Demo 6: Script & State Injection ===\n\n");

    engine->document.scripts_len = 0;
    strcat(engine->document.scripts, "<script src=\"/bundle.js\" defer></script>\n");
    strcat(engine->document.scripts, "<script src=\"/vendor.js\" defer></script>");
    engine->document.scripts_len = strlen(engine->document.scripts);

    strcpy(engine->document.global_state_json,
        "{\"user\":{\"name\":\"John\"},\"theme\":\"dark\",\"cart\":[]}");
    engine->document.global_state_len = strlen(engine->document.global_state_json);

    char html[SSR_HTML_BUFFER_SIZE];
    size_t html_len;
    ssr_render_page(engine, "/", html, &html_len);

    ssr_inject_scripts(engine, html, &html_len);
    ssr_inject_state(engine, html, &html_len);

    printf("Full HTML with scripts & state:\n");
    printf("%s\n", html);
}

int main(void) {
    printf("=== SSR/SSG Engine Demonstration ===\n");
    printf("====================================\n\n");

    SSREngine engine;
    ssr_init(&engine);

    ssr_set_mode(&engine, SSR_RENDER_STATIC);

    printf("Server document template initialized.\n");
    printf("Initial HTML head size: %zu bytes\n", engine.document.head_len);

    define_components(&engine);
    define_routes(&engine);

    demo_static_render(&engine);

    demo_page_render(&engine);

    demo_hydration(&engine);

    demo_isr(&engine);

    demo_static_generation(&engine);

    demo_script_injection(&engine);

    printf("\n====================================\n");
    printf("Final Statistics:\n");
    ssr_print_stats(&engine);
    printf("====================================\n");
    printf("SSR/SSG Demo Complete\n");

    return 0;
}
