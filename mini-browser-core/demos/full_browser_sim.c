#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "render_pipeline.h"
#include "event_loop.h"
#include "composite_layer.h"
#include "js_engine_sim.h"
#include "concurrency_browser.h"

typedef struct {
    RenderPipeline rp;
    EventLoop      el;
    Compositor     comp;
    Isolate        js_iso;
    BrowserModel   bm;
    uint64_t       sim_time_ms;
    uint64_t       frame_number;
    bool           running;
} BrowserSim;

static BrowserSim g_sim;

#define LOG(fmt, ...) printf("[%06llu] " fmt "\n", (unsigned long long)g_sim.sim_time_ms, ##__VA_ARGS__)

static void on_style_stage(PipelineStage stage, void *data) {
    (void)data;
    LOG("  Pipeline stage: %s", rp_stage_name(stage));
}

static void on_raf(uint64_t ts_ms, void *data) {
    (void)data;
    LOG("  RAF callback fired @ %llu ms", (unsigned long long)ts_ms);
}

static void background_fetch(void *data) {
    LOG("  [Network] fetch('%s') started", (const char *)data);
}

static void click_handler(void *data) {
    LOG("  [Input] click on element #%s", (const char *)data);
}

static void promise_resolve(void *data) {
    LOG("  [Microtask] Promise resolved: %s", (const char *)data);
}

static void idle_cb(uint64_t deadline, bool timeout, void *data) {
    LOG("  [Idle] deadline=%llu timeout=%d work=%s",
        (unsigned long long)deadline, timeout, (const char *)data);
}

static void init_simulation(void) {
    memset(&g_sim, 0, sizeof(g_sim));
    g_sim.sim_time_ms = 0;
    g_sim.frame_number = 0;
    g_sim.running = true;

    rp_init(&g_sim.rp, 60.0);
    rp_register_hook(&g_sim.rp, STYLE_RECALC, on_style_stage, NULL);
    rp_register_hook(&g_sim.rp, LAYOUT_BUILD, on_style_stage, NULL);
    rp_register_hook(&g_sim.rp, PAINT_GENERATE, on_style_stage, NULL);
    rp_register_hook(&g_sim.rp, COMPOSITE_MERGE, on_style_stage, NULL);

    el_init(&g_sim.el);
    el_enable_long_task_detection(&g_sim.el, true);
    el_enable_starvation_prevention(&g_sim.el, true);
    el_set_micro_starvation_limit(&g_sim.el, 32);

    comp_init(&g_sim.comp, 1920, 1080, 2.0f);
    comp_set_impl_side_painting(&g_sim.comp, true);
    comp_set_gpu_raster(&g_sim.comp, true);

    js_isolate_init(&g_sim.js_iso, 1);
    js_isolate_set_main_thread(&g_sim.js_iso, true);

    bm_init(&g_sim.bm);
    bm_setup_default(&g_sim.bm);
}

static void build_page_layers(void) {
    Compositor *c = &g_sim.comp;

    int32_t header = comp_create_layer(c, LAYER_PAINT, c->root_layer_idx);
    Layer *hl = comp_get_layer(c, header);
    comp_set_layer_bounds(hl, 0, 0, 1920, 80);
    comp_set_layer_position(hl, 0, 0);
    comp_add_compositing_reason(hl, COMP_REASON_FIXED_POSITION);

    int32_t content = comp_create_layer(c, LAYER_SCROLL, c->root_layer_idx);
    Layer *cl = comp_get_layer(c, content);
    comp_set_layer_bounds(cl, 0, 80, 1920, 2000);
    comp_set_layer_position(cl, 0, 80);
    cl->is_scrollable = true;

    int32_t sidebar = comp_create_layer(c, LAYER_GRAPHICS, c->root_layer_idx);
    Layer *sl = comp_get_layer(c, sidebar);
    comp_set_layer_bounds(sl, 0, 80, 300, 1000);
    comp_set_layer_position(sl, 0, 80);
    comp_add_compositing_reason(sl, COMP_REASON_STACKING_CONTEXT);
    comp_set_layer_opacity(sl, 0.95f);

    int32_t video = comp_create_layer(c, LAYER_VIDEO, content);
    Layer *vl = comp_get_layer(c, video);
    comp_set_layer_bounds(vl, 50, 100, 640, 360);
    comp_set_layer_position(vl, 50, 180);
    comp_add_compositing_reason(vl, COMP_REASON_VIDEO);
    comp_add_compositing_reason(vl, COMP_REASON_OPACITY_ANIM);
    comp_set_layer_opacity(vl, 0.90f);

    int32_t iframe_layer = comp_create_layer(c, LAYER_IFRAME, content);
    Layer *il = comp_get_layer(c, iframe_layer);
    comp_set_layer_bounds(il, 800, 200, 400, 600);
    comp_set_layer_position(il, 800, 280);
    comp_add_compositing_reason(il, COMP_REASON_IFRAME);

    int32_t canvas = comp_create_layer(c, LAYER_WEBGL, content);
    Layer *wl = comp_get_layer(c, canvas);
    comp_set_layer_bounds(wl, 100, 500, 800, 600);
    comp_set_layer_position(wl, 100, 580);
    comp_add_compositing_reason(wl, COMP_REASON_WEBGL);
    comp_add_compositing_reason(wl, COMP_REASON_TRANSFORM_3D);

    int32_t overlay = comp_create_layer(c, LAYER_OVERLAY, c->root_layer_idx);
    Layer *ol = comp_get_layer(c, overlay);
    comp_set_layer_bounds(ol, 0, 0, 1920, 1080);
    comp_set_layer_position(ol, 0, 0);
    comp_add_compositing_reason(ol, COMP_REASON_ANIMATION);
    ol->opacity = 0.5f;
    ol->z_index = 1000.0f;

    comp_tile_layer(c, header);
    comp_tile_layer(c, content);
    comp_tile_layer(c, sidebar);
    comp_tile_layer(c, video);
    comp_tile_layer(c, iframe_layer);
    comp_tile_layer(c, canvas);
    comp_tile_layer(c, overlay);
}

static void schedule_initial_tasks(void) {
    EventLoop *el = &g_sim.el;
    el_set_time(el, g_sim.sim_time_ms);

    el_schedule_task(el, TASK_FETCH, background_fetch, "/api/data.json", 0);
    el_schedule_task(el, TASK_FETCH, background_fetch, "/api/config.js", 5);
    el_schedule_task(el, TASK_SETTIMEOUT, background_fetch, "delayed-init", 100);

    el_schedule_task(el, TASK_EVENT_SCROLL, click_handler, "scrollArea", 0);
    el_schedule_task(el, TASK_EVENT_CLICK, click_handler, "btn-submit", 20);
    el_schedule_task(el, TASK_EVENT_RESIZE, click_handler, "resizeObs", 50);

    el_queue_microtask(el, MICRO_PROMISE_THEN, promise_resolve, "fetch-complete");
    el_queue_microtask(el, MICRO_PROMISE_THEN, promise_resolve, "layout-done");
    el_queue_microtask(el, MICRO_QUEUE_MICROTASK, promise_resolve, "queueMicrotask");
    el_queue_microtask(el, MICRO_MUTATION_OBSERVER, promise_resolve, "dom-mutation");

    el_request_idle_callback(el, idle_cb, "cleanup-cache");
    el_request_idle_callback(el, idle_cb, "report-analytics");
}

static void simulate_frame_work(void) {
    EventLoop *el = &g_sim.el;
    RenderPipeline *rp = &g_sim.rp;

    uint64_t frame_start_us = g_sim.sim_time_ms * 1000;

    LOG("---- Frame %llu BEGIN (t=%llums) ----",
        (unsigned long long)g_sim.frame_number,
        (unsigned long long)g_sim.sim_time_ms);

    rp_request_animation_frame(rp, on_raf, NULL);

    for (int sub_tick = 0; sub_tick < 5; sub_tick++) {
        g_sim.sim_time_ms += 3;
        el_set_time(el, g_sim.sim_time_ms);
        el_tick(el, g_sim.sim_time_ms);
    }

    rp_full_frame(rp, frame_start_us);
    FrameInfo fi = rp_get_frame_info(rp);

    Compositor *c = &g_sim.comp;
    comp_calculate_visible_tiles(c, 0, g_sim.sim_time_ms * 0.1f);
    comp_update_tile_priorities(c);
    comp_rasterize_dirty(c);
    comp_commit_frame(c);

    LOG("---- Frame %llu END | state=%d duration=%.2fms util=%.1f%% ----",
        (unsigned long long)fi.frame_id, fi.state,
        rp_frame_duration_ms(&fi), rp_get_utilization(rp) * 100.0);

    g_sim.frame_number++;
}

static void simulate_browser_ipc(void) {
    BrowserModel *bm = &g_sim.bm;
    BrowserProcess *gpu = bm_get_gpu(bm);

    if (gpu) {
        uint64_t ch_id = 0;
        for (uint32_t i = 0; i < bm->channel_count; i++) {
            if (bm->channels[i].target_pid == gpu->pid) {
                ch_id = bm->channels[i].channel_id;
                break;
            }
        }
        if (ch_id) {
            IPCChannel *ch = bm_get_channel(bm, ch_id);
            if (ch) {
                LOG("  [IPC] Sending frame to GPU process (channel %llu)",
                    (unsigned long long)ch_id);
                bm_send_message(ch, IPC_MSG_NOTIFICATION,
                                (const uint8_t *)"commitFrame", 12, false);
                IPCMessage msgs[4];
                uint32_t n;
                bm_recv_messages(ch, msgs, 4, &n);
                LOG("  [IPC] GPU acknowledged %u messages", n);
            }
        }
    }
}

static void simulate_js_work(void) {
    Isolate *iso = &g_sim.js_iso;
    LOG("  [JIT] tier=%s call_count=%llu",
        js_jit_tier_name(iso->jit.current_tier),
        (unsigned long long)iso->jit.call_count);

    for (int i = 0; i < 50; i++) js_jit_on_call(iso, 0xBEEF000 + i);
    for (int i = 0; i < 200; i++) js_jit_on_call(iso, 0xBEEF000);
    if (iso->jit.current_tier == JIT_IGNITION)
        LOG("  [JIT] upgraded to %s after hot function calls",
            js_jit_tier_name(iso->jit.current_tier));

    const char *props[] = { "id", "className", "style", "dataset", "offsetParent" };
    HiddenClass *shape = js_create_shape(iso, props, 5);
    for (int i = 0; i < 10; i++) {
        JSObject *obj = js_create_object(iso, shape);
        js_set_property(iso, obj, 0, js_make_smi(iso, 100 + i));
        js_set_property(iso, obj, 1, js_make_string(iso, "highlighted"));
        js_set_property(iso, obj, 2, js_make_heap_number(iso, 3.14 + i));
    }
    LOG("  [JS] Created %u objects with %u shapes",
        iso->object_count, iso->shape_count);

    if (iso->object_count > 200) {
        js_gc_trigger_scavenge(iso);
        LOG("  [GC] Scavenge: surviving %llu objects",
            (unsigned long long)iso->gc.survived_objects);
        const MemoryStats *ms = js_get_memory_stats(iso);
        LOG("  [GC] Pause: %lluus total; %llu GCs performed",
            (unsigned long long)ms->gc_pause_total_us,
            (unsigned long long)ms->gc_count);
    }
}

static void print_report(void) {
    printf("\n========== FINAL REPORT ==========\n\n");

    printf("--- Event Loop ---\n");
    TaskMetrics tm = el_get_last_metrics(&g_sim.el);
    printf("  Tasks processed: %llu\n",
           (unsigned long long)el_get_total_processed(&g_sim.el));
    printf("  Long tasks: %llu\n",
           (unsigned long long)el_get_long_task_count(&g_sim.el));
    printf("  Last task: type=%d elapsed=%llums isLong=%d\n",
           tm.task_type, (unsigned long long)tm.elapsed_ms, tm.is_long_task);

    printf("\n--- Rendering Pipeline ---\n");
    FrameInfo fi = rp_get_frame_info(&g_sim.rp);
    printf("  Frames: %llu\n",
           (unsigned long long)g_sim.rp.frame_counter);
    printf("  Last frame: id=%llu state=%d duration=%.2fms budget=%.2fms\n",
           (unsigned long long)fi.frame_id, fi.state,
           rp_frame_duration_ms(&fi),
           (double)g_sim.rp.frame_budget_us / 1000.0);

    printf("\n--- Compositor ---\n");
    printf("  Layers: %d\n", g_sim.comp.layer_count);
    printf("  Tiles rasterized: %llu\n",
           (unsigned long long)g_sim.comp.tiles_rasterized);
    printf("  Total memory: %llu bytes\n",
           (unsigned long long)comp_get_total_memory(&g_sim.comp));

    printf("\n--- Browser Process Model ---\n");
    printf("  Processes: %u\n", g_sim.bm.process_count);
    printf("  IPC channels: %u\n", g_sim.bm.channel_count);
    uint64_t total_mem;
    uint64_t pp[64];
    bm_get_memory_report(&g_sim.bm, &total_mem, pp);
    printf("  Total memory: %llu MB\n",
           (unsigned long long)(total_mem / (1024 * 1024)));

    printf("\n--- JS Engine ---\n");
    const MemoryStats *ms = js_get_memory_stats(&g_sim.js_iso);
    printf("  JIT tier: %s\n", js_jit_tier_name(g_sim.js_iso.jit.current_tier));
    printf("  Shapes: %u\n", g_sim.js_iso.shape_count);
    printf("  Objects: %u\n", g_sim.js_iso.object_count);
    printf("  GC count: %llu | IC caches: %u\n",
           (unsigned long long)ms->gc_count, g_sim.js_iso.jit.ic_count);

    printf("\n==================================\n");
}

static void cleanup(void) {
    comp_destroy(&g_sim.comp);
    rp_destroy(&g_sim.rp);
    el_destroy(&g_sim.el);
    js_isolate_destroy(&g_sim.js_iso);
    bm_destroy(&g_sim.bm);
}

int main(void) {
    printf("=== mini-browser-core FULL DEMO ===\n");
    printf("=== All 5 modules integrated ===\n\n");

    init_simulation();

    LOG("Initialization complete. Building page...");
    build_page_layers();
    schedule_initial_tasks();

    g_sim.bm.total_ipc_messages = 0;
    g_sim.bm.total_memory_all = 0;

    for (int frame = 0; frame < 10 && g_sim.running; frame++) {
        simulate_frame_work();
        simulate_browser_ipc();
        simulate_js_work();
    }

    print_report();
    cleanup();

    printf("\nAll done.\n");
    return 0;
}
