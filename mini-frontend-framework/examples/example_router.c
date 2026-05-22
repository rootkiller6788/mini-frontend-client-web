#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spa_router.h"
#include "virtual_dom.h"
#include "component_model.h"

static VNode* home_page(void* params) {
    (void)params;
    VNode* div = vdom_create_element("div");
    vdom_set_prop(div, "class", "page home-page");

    VNode* h1 = vdom_create_element("h1");
    vdom_set_text(h1, "Home Page");
    vdom_append_child(div, h1);

    VNode* p = vdom_create_element("p");
    vdom_set_text(p, "Welcome to the SPA Router demo! Navigate using the links below.");
    vdom_append_child(div, p);

    VNode* nav = vdom_create_element("nav");
    vdom_set_prop(nav, "class", "page-nav");

    const char* links[] = {"/about", "/users", "/contact"};
    const char* labels[] = {"About", "Users", "Contact"};
    for (int i = 0; i < 3; i++) {
        VNode* a = vdom_create_element("a");
        vdom_set_prop(a, "href", links[i]);
        char nav_evt[64];
        snprintf(nav_evt, sizeof(nav_evt), "navigate:%s", links[i]);
        vdom_set_event(a, "onclick", nav_evt);
        vdom_set_text(a, labels[i]);
        vdom_append_child(nav, a);
    }
    vdom_append_child(div, nav);

    return div;
}

static VNode* about_page(void* params) {
    (void)params;
    VNode* div = vdom_create_element("div");
    vdom_set_prop(div, "class", "page about-page");

    VNode* h1 = vdom_create_element("h1");
    vdom_set_text(h1, "About");
    vdom_append_child(div, h1);

    VNode* section = vdom_create_element("section");
    vdom_set_prop(section, "class", "about-content");

    VNode* p1 = vdom_create_element("p");
    vdom_set_text(p1, "Mini Frontend Framework is a C99 implementation of modern frontend concepts.");
    vdom_append_child(section, p1);

    VNode* h3 = vdom_create_element("h3");
    vdom_set_text(h3, "Features");
    vdom_append_child(section, h3);

    VNode* ul = vdom_create_element("ul");
    const char* features[] = {"Virtual DOM with keyed reconciliation",
                               "React-style Hooks (useState, useEffect, useRef, useMemo)",
                               "Component model with lifecycle",
                               "SPA Router with History API",
                               "Reactive Signals system"};
    for (int i = 0; i < 5; i++) {
        VNode* li = vdom_create_element("li");
        vdom_set_text(li, features[i]);
        vdom_append_child(ul, li);
    }
    vdom_append_child(section, ul);
    vdom_append_child(div, section);

    VNode* back = vdom_create_element("a");
    vdom_set_prop(back, "href", "/");
    vdom_set_event(back, "onclick", "navigate:/");
    vdom_set_text(back, "< Back to Home");
    vdom_append_child(div, back);

    return div;
}

static VNode* users_list_page(void* params) {
    (void)params;
    VNode* div = vdom_create_element("div");
    vdom_set_prop(div, "class", "page users-page");

    VNode* h1 = vdom_create_element("h1");
    vdom_set_text(h1, "Users");
    vdom_append_child(div, h1);

    VNode* ul = vdom_create_element("ul");
    vdom_set_prop(ul, "class", "user-list");

    const char* users[] = {"Alice Johnson", "Bob Smith", "Charlie Brown", "Diana Prince", "Eve Wilson"};
    for (int i = 0; i < 5; i++) {
        VNode* li = vdom_create_element("li");

        VNode* a = vdom_create_element("a");
        char user_path[64];
        snprintf(user_path, sizeof(user_path), "/users/%d", i + 1);
        vdom_set_prop(a, "href", user_path);
        char nav_evt[64];
        snprintf(nav_evt, sizeof(nav_evt), "navigate:%s", user_path);
        vdom_set_event(a, "onclick", nav_evt);
        vdom_set_text(a, users[i]);
        vdom_append_child(li, a);

        vdom_append_child(ul, li);
    }
    vdom_append_child(div, ul);

    VNode* back = vdom_create_element("a");
    vdom_set_prop(back, "href", "/");
    vdom_set_event(back, "onclick", "navigate:/");
    vdom_set_text(back, "< Back to Home");
    vdom_append_child(div, back);

    return div;
}

static VNode* user_detail_page(void* params) {
    RouteParams* rp = (RouteParams*)params;
    const char* user_id = router_get_param(rp, "id");

    VNode* div = vdom_create_element("div");
    vdom_set_prop(div, "class", "page user-detail-page");

    VNode* h1 = vdom_create_element("h1");
    char title[64];
    snprintf(title, sizeof(title), "User Profile: %s", user_id ? user_id : "unknown");
    vdom_set_text(h1, title);
    vdom_append_child(div, h1);

    VNode* card = vdom_create_element("div");
    vdom_set_prop(card, "class", "profile-card");

    const char* details[] = {"Name: Dynamic User", "Email: user@example.com", "Role: Member"};
    for (int i = 0; i < 3; i++) {
        VNode* p = vdom_create_element("p");
        vdom_set_text(p, details[i]);
        vdom_append_child(card, p);
    }
    vdom_append_child(div, card);

    VNode* back = vdom_create_element("a");
    vdom_set_prop(back, "href", "/users");
    vdom_set_event(back, "onclick", "navigate:/users");
    vdom_set_text(back, "< Back to Users");
    vdom_append_child(div, back);

    return div;
}

static VNode* contact_page(void* params) {
    (void)params;
    VNode* div = vdom_create_element("div");
    vdom_set_prop(div, "class", "page contact-page");

    VNode* h1 = vdom_create_element("h1");
    vdom_set_text(h1, "Contact");
    vdom_append_child(div, h1);

    VNode* form = vdom_create_element("form");
    vdom_set_prop(form, "class", "contact-form");

    const char* fields[] = {"name", "email", "message"};
    const char* labels[] = {"Name:", "Email:", "Message:"};
    for (int i = 0; i < 3; i++) {
        VNode* label = vdom_create_element("label");
        vdom_set_prop(label, "for", fields[i]);
        vdom_set_text(label, labels[i]);
        vdom_append_child(form, label);

        if (i < 2) {
            VNode* input = vdom_create_element("input");
            vdom_set_prop(input, "type", i == 1 ? "email" : "text");
            vdom_set_prop(input, "name", fields[i]);
            vdom_set_prop(input, "id", fields[i]);
            vdom_append_child(form, input);
        } else {
            VNode* textarea = vdom_create_element("textarea");
            vdom_set_prop(textarea, "name", fields[i]);
            vdom_set_prop(textarea, "id", fields[i]);
            vdom_append_child(form, textarea);
        }
    }

    VNode* submit = vdom_create_element("button");
    vdom_set_prop(submit, "type", "submit");
    vdom_set_text(submit, "Send Message");
    vdom_append_child(form, submit);

    vdom_append_child(div, form);

    VNode* back = vdom_create_element("a");
    vdom_set_prop(back, "href", "/");
    vdom_set_event(back, "onclick", "navigate:/");
    vdom_set_text(back, "< Back to Home");
    vdom_append_child(div, back);

    return div;
}

static VNode* not_found_page(void* params) {
    (void)params;
    VNode* div = vdom_create_element("div");
    vdom_set_prop(div, "class", "page not-found-page");

    VNode* h1 = vdom_create_element("h1");
    vdom_set_text(h1, "404 - Page Not Found");
    vdom_append_child(div, h1);

    VNode* p = vdom_create_element("p");
    vdom_set_text(p, "The page you are looking for does not exist.");
    vdom_append_child(div, p);

    VNode* a = vdom_create_element("a");
    vdom_set_prop(a, "href", "/");
    vdom_set_event(a, "onclick", "navigate:/");
    vdom_set_text(a, "Go to Home");
    vdom_append_child(div, a);

    return div;
}

static bool auth_guard(void* params, void* context) {
    (void)params;
    (void)context;
    return true;
}

static void demo_route_matching(void) {
    printf("\n--- Route Matching Demo ---\n");

    Router* router = router_create(ROUTER_HISTORY);

    router_add_route(router, "/", "home", ROUTE_EXACT, home_page);
    router_add_route(router, "/about", "about", ROUTE_EXACT, about_page);
    router_add_route(router, "/users", "users", ROUTE_EXACT, users_list_page);
    router_add_route(router, "/users/:id", "user-detail", ROUTE_PARAM, user_detail_page);
    router_add_route(router, "/contact", "contact", ROUTE_EXACT, contact_page);
    router_set_not_found(router, "*", not_found_page);

    const char* test_paths[] = {"/", "/about", "/users", "/users/42", "/contact", "/nonexistent"};
    for (int i = 0; i < 6; i++) {
        RouteMatch match = router_match(router, test_paths[i]);
        printf("  '%s' -> %s", test_paths[i],
               match.found ? match.route->name : "NOT FOUND");
        if (match.found && match.params.count > 0) {
            printf(" (params:");
            for (int j = 0; j < match.params.count; j++) {
                printf(" %s=%s", match.params.keys[j], match.params.values[j]);
            }
            printf(")");
        }
        printf("\n");
    }

    router_free(router);
}

static void demo_nested_routes(void) {
    printf("\n--- Nested Routes Demo ---\n");

    Router* router = router_create(ROUTER_HASH);
    int admin_idx = router_add_route(router, "/admin", "admin", ROUTE_PREFIX, NULL);

    Route* admin = &router->routes[admin_idx];

    Route dashboard_route = { .path = "/dashboard", .match_type = ROUTE_EXACT };
    strncpy(dashboard_route.name, "admin-dashboard", sizeof(dashboard_route.name) - 1);
    dashboard_route.component = home_page;
    router_add_nested_route(admin, &dashboard_route);

    Route settings_route = { .path = "/settings", .match_type = ROUTE_EXACT };
    strncpy(settings_route.name, "admin-settings", sizeof(settings_route.name) - 1);
    settings_route.component = about_page;
    router_add_nested_route(admin, &settings_route);

    printf("  Parent route: %s (%d children)\n", admin->name, admin->child_count);
    for (int i = 0; i < admin->child_count; i++) {
        printf("    Child: %s [%s]\n", admin->children[i]->name, admin->children[i]->path);
        if (admin->children[i]->component) {
            VNode* rendered_child = admin->children[i]->component(NULL);
            printf("      Rendered: %d children\n", rendered_child->child_count);
            vdom_free_recursive(rendered_child);
        }
    }

    router_free(router);
}

static void demo_route_guards(void) {
    printf("\n--- Route Guards Demo ---\n");

    Router* router = router_create(ROUTER_HISTORY);

    RouteGuard auth_guard_struct = { .check = auth_guard };
    strncpy(auth_guard_struct.name, "auth", sizeof(auth_guard_struct.name) - 1);

    RouteGuard* guards[] = { &auth_guard_struct };
    router_add_route_ex(router, "/dashboard", "dashboard", ROUTE_EXACT,
                        home_page, guards, 1);

    printf("  Route '/dashboard' with auth guard created\n");
    printf("  Guard check result: %s\n",
           router_run_guards(&router->routes[0], NULL, NULL) ? "passed" : "blocked");

    router_free(router);
}

static void demo_history_api(void) {
    printf("\n--- History API Demo ---\n");

    router_history_push("/home");
    router_history_push("/users");
    router_history_push("/users/1");

    printf("  History length: %d\n", router_history_length());
    printf("  Current path: %s\n", "/users/1");

    router_history_back();
    printf("  After back(): %s\n", "/users");

    router_history_back();
    printf("  After back(): %s\n", "/home");

    router_history_forward();
    printf("  After forward(): %s\n", "/users");
}

int main(void) {
    printf("========================================\n");
    printf("  Mini Frontend Framework - Router Example\n");
    printf("========================================\n");

    printf("\n[1] Creating router...\n");
    Router* router = router_create(ROUTER_HISTORY);
    printf("  Router created (mode=%d)\n", router->mode);

    printf("\n[2] Adding routes...\n");
    router_add_route(router, "/", "home", ROUTE_EXACT, home_page);
    router_add_route(router, "/about", "about", ROUTE_EXACT, about_page);
    router_add_route(router, "/users", "users", ROUTE_EXACT, users_list_page);
    router_add_route(router, "/users/:id", "user-detail", ROUTE_PARAM, user_detail_page);
    router_add_route(router, "/contact", "contact", ROUTE_EXACT, contact_page);
    router_add_route(router, "/posts/*", "posts", ROUTE_WILDCARD, home_page);
    router_set_not_found(router, "*", not_found_page);
    printf("  Added %d routes + 404 handler\n", router->route_count);

    printf("\n[3] Testing navigation...\n");
    const char* nav_paths[] = {"/", "/about", "/users", "/users/99", "/contact"};
    for (int i = 0; i < 5; i++) {
        router_navigate(router, nav_paths[i]);
        const char* current = router_get_current_path(router);
        Route* current_route = router_get_current_route(router);
        printf("  Navigate to '%s' -> current='%s' route='%s'\n",
               nav_paths[i], current,
               current_route ? current_route->name : "null");
    }

    printf("\n[4] Testing 404...\n");
    router_navigate(router, "/does-not-exist");
    Route* nf_route = router_get_current_route(router);
    printf("  Navigate to '/does-not-exist' -> route='%s'\n",
           nf_route ? nf_route->name : "null");
    if (nf_route && nf_route->component) {
        VNode* nf_page = nf_route->component(NULL);
        printf("  404 page rendered with %d children\n", nf_page->child_count);
        vdom_free_recursive(nf_page);
    }

    demo_route_matching();
    demo_nested_routes();
    demo_route_guards();
    demo_history_api();

    printf("\n[5] Rendering current route...\n");
    router_navigate(router, "/");
    VNode* rendered = router_render(router);
    if (rendered) {
        printf("  Rendered home page:\n");
        vdom_debug_print(rendered, 0);
        vdom_free_recursive(rendered);
    }

    router_free(router);
    printf("\n=== Router Example Completed ===\n");
    return 0;
}
