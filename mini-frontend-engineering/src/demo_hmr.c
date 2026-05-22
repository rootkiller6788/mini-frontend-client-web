#include "hmr_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char id[64];
    char source[1024];
    int render_count;
    int state_value;
} AppComponent;

static AppComponent header = {"header", "function Header() { return '<h1>My App</h1>'; }", 0, 0};
static AppComponent sidebar = {"sidebar", "function Sidebar() { return '<nav>Menu</nav>'; }", 0, 0};
static AppComponent main_comp = {"main", "function Main() { return '<div>Content v1.0</div>'; }", 0, 42};
static AppComponent footer = {"footer", "function Footer() { return '<footer>2024</footer>'; }", 0, 0};
static AppComponent counter = {"counter", "function Counter() { return '<button>Count:0</button>'; }", 0, 0};

static void sim_render(AppComponent *comp) {
    comp->render_count++;
    printf("  Rendering [%s] (version %d): %s\n", comp->id, comp->render_count, comp->source);
}

static void sim_update_source(AppComponent *comp, const char *new_src) {
    strncpy(comp->source, new_src, 1023);
    printf("  Source updated for [%s]: %s\n", comp->id, comp->source);
}

static void register_app_modules(HMREngine *engine) {
    const char *modules[] = {"header", "sidebar", "main", "footer", "counter"};
    for (int i = 0; i < 5; i++) {
        hmr_register_module(engine, modules[i], (uint64_t)(i + 100));
    }

    hmr_add_dependency(engine, "main", "sidebar");
    hmr_add_dependency(engine, "main", "footer");
    hmr_add_dependency(engine, "sidebar", "counter");
    hmr_add_dependency(engine, "header", "counter");

    hmr_set_self_accept(engine, "main", true);
    hmr_set_self_accept(engine, "counter", true);

    printf("\nModule graph:\n");
    printf("  main -> {sidebar, footer}\n");
    printf("  sidebar -> {counter}\n");
    printf("  header -> {counter}\n");
    printf("  Self-accepting: main, counter\n");
}

static void sim_hot_update(HMREngine *engine) {
    printf("\n--- Simulating Hot Update ---\n");
    printf("Developer edits main component in IDE...\n");

    const char *update_payload =
        "{\"type\":\"update\",\"module\":\"main\",\"hash\":\"abc123\","
        "\"source\":\"function Main() { return '<div>Content v2.0 - HOT UPDATE!</div>'; }\"}";

    int idx = hmr_handle_update(engine, update_payload);
    if (idx >= 0) {
        printf("Update queued at index %d\n", idx);

        hmr_collect_boundary_modules(engine, "main", &engine->current_boundary);
        if (engine->current_boundary.needs_full_reload)
            printf("WARNING: No HMR boundary found - full reload needed!\n");
        else
            printf("HMR boundary found at: %s\n", engine->current_boundary.boundary_module);
    }

    hmr_apply_updates(engine);

    sim_update_source(&main_comp, "function Main() { return '<div>Content v2.0 - HOT UPDATE!</div>'; }");
    sim_render(&main_comp);
    printf("  State preserved: state_value = %d\n", main_comp.state_value);
}

static void sim_counter_update(HMREngine *engine) {
    printf("\n--- Simulating Counter Hot Update ---\n");
    counter.state_value = 99;
    printf("  Current counter state: %d\n", counter.state_value);

    const char *update_payload =
        "{\"type\":\"update\",\"module\":\"counter\",\"hash\":\"def456\","
        "\"source\":\"function Counter() { return '<button class=\\\\"new-style\\\">Count:99</button>'; }\"}";

    hmr_handle_update(engine, update_payload);

    hmr_collect_boundary_modules(engine, "counter", &engine->current_boundary);
    if (engine->current_boundary.needs_full_reload)
        printf("WARNING: No HMR boundary - full reload needed!\n");
    else
        printf("HMR boundary: %s (self-accepting)\n", engine->current_boundary.boundary_module);

    hmr_apply_updates(engine);

    sim_update_source(&counter, "function Counter() { return '<button class=\"new-style\">Count:99</button>'; }");
    printf("  Counter updated! State preserved: %d\n", counter.state_value);
}

static void sim_full_reload_scenario(HMREngine *engine) {
    printf("\n--- Simulating Full Reload Scenario ---\n");
    printf("Developer changes sidebar, which has no self-accept...\n");

    const char *update_payload =
        "{\"type\":\"update\",\"module\":\"sidebar\",\"hash\":\"ghi789\","
        "\"source\":\"function Sidebar() { return '<nav>Updated Menu</nav>'; }\"}";

    hmr_handle_update(engine, update_payload);
    hmr_apply_updates(engine);

    printf("  Full reload would be triggered (no accepting parent for sidebar)\n");
    printf("  All component state would be lost\n");
}

static void sim_state_snapshot(HMREngine *engine) {
    printf("\n--- State Snapshot Demo ---\n");
    printf("Taking snapshot before update...\n");
    hmr_save_state_snapshot(engine);
    printf("  Snapshot %d saved\n", engine->snapshot_count);

    counter.state_value = 150;
    printf("  Modified counter state: %d\n", counter.state_value);

    printf("Restoring previous state...\n");
    hmr_restore_state(engine, engine->snapshot_count - 1);
    printf("  State restoration simulated\n");

    hmr_save_state_snapshot(engine);
    printf("  Snapshots stored: %d\n", engine->snapshot_count);
}

static void sim_disconnect_reconnect(HMREngine *engine) {
    printf("\n--- Disconnect / Reconnect Simulation ---\n");
    printf("Dev server stopped...\n");
    hmr_disconnect(engine);
    printf("  State: %s\n", hmr_state_string(engine->state));

    printf("Dev server restarts...\n");
    hmr_connect(engine);
    printf("  State: %s\n", hmr_state_string(engine->state));
}

static void sim_multi_update(HMREngine *engine) {
    printf("\n--- Multiple Simultaneous Updates ---\n");

    const char *updates[] = {
        "{\"type\":\"update\",\"module\":\"header\",\"source\":\"function Header() { return '<h1>New Header</h1>'; }\"}",
        "{\"type\":\"update\",\"module\":\"footer\",\"source\":\"function Footer() { return '<footer>2025</footer>'; }\"}",
        "{\"type\":\"update\",\"module\":\"main\",\"source\":\"function Main() { return '<div>v3.0</div>'; }\"}",
    };

    for (int i = 0; i < 3; i++) {
        hmr_handle_update(engine, updates[i]);
    }
    printf("  %d updates queued\n", engine->pending_count);
    hmr_apply_updates(engine);
}

int main(void) {
    printf("=== HMR Engine Demonstration ===\n");
    printf("================================\n");

    HMREngine engine;
    hmr_init(&engine, "ws://localhost:3000/hmr");

    printf("Initial State: %s\n\n", hmr_state_string(engine.state));

    printf("--- Step 1: Connect to Dev Server ---\n");
    hmr_connect(&engine);
    printf("Connected: %s | State: %s\n",
        engine.connected ? "yes" : "no", hmr_state_string(engine.state));

    printf("\n--- Step 2: Register Application Modules ---\n");
    register_app_modules(&engine);

    printf("\n--- Step 3: Initial Render ---\n");
    AppComponent *comps[] = {&header, &sidebar, &main_comp, &footer, &counter};
    for (int i = 0; i < 5; i++) sim_render(comps[i]);

    sim_hot_update(&engine);

    sim_counter_update(&engine);

    sim_full_reload_scenario(&engine);

    sim_state_snapshot(&engine);

    sim_disconnect_reconnect(&engine);

    sim_multi_update(&engine);

    printf("\n--- Edge Case: Update Unknown Module ---\n");
    const char *unknown_update =
        "{\"type\":\"update\",\"module\":\"nonexistent\",\"source\":\"// nothing\"}";
    int uk_idx = hmr_handle_update(&engine, unknown_update);
    printf("  Update for unknown module returned index %d\n", uk_idx);

    printf("\n--- Edge Case: Corrupted Payload ---\n");
    int bad_idx = hmr_handle_update(&engine, "not valid json at all {{{");
    printf("  Bad payload returned index %d\n", bad_idx);

    printf("\n--- Stress Test: Rapid Toggle ---\n");
    for (int iter = 0; iter < 5; iter++) {
        hmr_save_state_snapshot(&engine);
        const char *toggle_payload =
            "{\"type\":\"update\",\"module\":\"main\",\"source\":\"function Main() { return '<div>toggle'; }\"}";
        hmr_handle_update(&engine, toggle_payload);
        hmr_apply_updates(&engine);
        printf("  Toggle %d: version=%llu, snapshots=%d\n",
            iter, (unsigned long long)engine.global_version, engine.snapshot_count);
    }

    printf("\n--- Reconnect After Full Reload ---\n");
    hmr_trigger_full_reload(&engine);
    printf("  State after full reload: %s\n", hmr_state_string(engine.state));
    hmr_connect(&engine);
    printf("  Reconnected: %s\n", engine.connected ? "yes" : "no");

    printf("\n--- Module Dependency Walk ---\n");
    for (int i = 0; i < engine.module_count; i++) {
        HMRModule *m = &engine.modules[i];
        printf("  %s (v%llu, self_accept=%d) -> [", m->module_id,
            (unsigned long long)m->version, m->self_accept);
        for (int d = 0; d < m->dependency_count; d++) {
            printf("%s%s", d > 0 ? ", " : "", m->dependency_ids[d]);
        }
        printf("]\n");
    }

    printf("\n--- Final Status ---\n");
    hmr_send_status(&engine);

    printf("\n================================\n");
    printf("HMR Demo Complete\n");
    printf("WebSocket URL: %s\n", engine.config.ws_url);
    printf("Total modules: %d\n", engine.module_count);
    printf("Updates applied: %d\n", engine.applied_updates);
    printf("Updates failed: %d\n", engine.failed_updates);
    printf("Global version: %llu\n", (unsigned long long)engine.global_version);
    printf("================================\n");

    printf("\n=== HMR CLI Simulation ===\n");
    printf("Starting dev server with HMR...\n");
    printf("  [dev] watching 12 files for changes...\n");
    printf("  [dev] server running at http://localhost:3000\n");
    printf("  [dev] HMR endpoint at ws://localhost:3000/hmr\n");
    printf("  [hmr] client connected\n");
    printf("  [hmr] modules registered: %d\n", engine.module_count);
    printf("  [hmr] awaiting changes...\n");
    printf("  [watch] main.js changed (98ms) -> hot update sent\n");
    printf("  [hmr] update applied in-place\n");
    printf("  [watch] styles.css changed (45ms) -> full reload (no JS boundary)\n");
    printf("  [hmr] fallback: page reloading...\n");

    return 0;
}
