#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "render_pipeline.h"
#include "event_loop.h"
#include "composite_layer.h"

static uint64_t g_total_raf_calls = 0;
static uint64_t g_total_frames = 0;
static uint64_t g_total_tasks = 0;
static uint64_t g_frame_drops = 0;
static uint64_t g_long_tasks = 0;

static void count_raf(uint64_t ts_ms, void *data) {
    g_total_raf_calls++;
    (void)ts_ms; (void)data;
}

static void benchmark_task(void *data) {
    volatile int sum = 0;
    int workload = *(int *)data;
    for (int i = 0; i < workload; i++) sum += i;
    g_total_tasks++;
    (void)sum;
}

static void benchmark_micro(void *data) {
    volatile int sum = 0;
    for (int i = 0; i < *(int *)data; i++) sum += i;
    (void)sum;
}

static void bench_event_loop(int duration_ms, int task_period_ms, int task_workload) {
    printf("\n--- Event Loop Benchmark (%dms, %dms period) ---\n", duration_ms, task_period_ms);

    EventLoop el;
    el_init(&el);
    el_enable_long_task_detection(&el, true);

    uint64_t start = 0;
    el_set_time(&el, start);

    g_total_tasks = 0;
    g_long_tasks = 0;

    int wl = task_workload;
    for (uint64_t t = 0; t < (uint64_t)duration_ms; t += (uint64_t)task_period_ms) {
        el_schedule_task(&el, TASK_SETTIMEOUT, benchmark_task, &wl,
                         t + (uint64_t)task_period_ms);
    }

    for (int i = 0; i < duration_ms / 10; i++) {
        el_queue_microtask(&el, MICRO_PROMISE_THEN, benchmark_micro, &wl);
    }

    for (uint64_t now = 0; now < (uint64_t)duration_ms; now += 2) {
        el_set_time(&el, now);
        el_tick(&el, now);
    }

    printf("  Tasks processed: %llu\n", (unsigned long long)g_total_tasks);
    printf("  Long tasks detected: %llu\n", (unsigned long long)el_get_long_task_count(&el));
    printf("  Total processed: %llu\n", (unsigned long long)el_get_total_processed(&el));
    g_long_tasks += el_get_long_task_count(&el);

    el_destroy(&el);
}

static void bench_render_pipeline(int frame_count, double fps) {
    printf("\n--- Render Pipeline Benchmark (%d frames @ %.0f FPS) ---\n", frame_count, fps);

    RenderPipeline rp;
    rp_init(&rp, fps);

    g_total_frames = 0;
    g_frame_drops = 0;
    g_total_raf_calls = 0;

    uint64_t interval_us = (uint64_t)(1e6 / fps);
    uint64_t sim_time_us = 0;

    for (int i = 0; i < frame_count; i++) {
        rp_request_animation_frame(&rp, count_raf, NULL);
        rp_full_frame(&rp, sim_time_us);
        FrameInfo fi = rp_get_frame_info(&rp);
        if (fi.missed_deadline) g_frame_drops++;
        g_total_frames++;
        sim_time_us += interval_us;
    }

    printf("  Frames: %llu\n", (unsigned long long)g_total_frames);
    printf("  Frame drops: %llu (%.1f%%)\n",
           (unsigned long long)g_frame_drops,
           g_total_frames ? 100.0 * g_frame_drops / g_total_frames : 0);
    printf("  Total RAF calls: %llu\n", (unsigned long long)g_total_raf_calls);
    printf("  Frame budget: %llu us\n", (unsigned long long)rp.frame_budget_us);
    printf("  Utilization: %.1f%%\n", rp_get_utilization(&rp) * 100.0);

    rp_destroy(&rp);
}

static void bench_compositor(void) {
    printf("\n--- Compositor Tile Benchmark ---\n");

    Compositor comp;
    comp_init(&comp, 1920, 1080, 1.0f);

    for (int i = 0; i < 50; i++) {
        int32_t lid = comp_create_layer(&comp, LAYER_GRAPHICS, comp.root_layer_idx);
        Layer *l = comp_get_layer(&comp, lid);
        int x = (i * 97) % 1920;
        int y = (i * 113) % 2000;
        comp_set_layer_bounds(l, (float)x, (float)y,
                             256.0f + (float)(i % 3) * 128.0f,
                             256.0f + (float)(i % 5) * 64.0f);
        if (i % 3 == 0) comp_add_compositing_reason(l, COMP_REASON_OPACITY_ANIM);
        if (i % 7 == 0) comp_add_compositing_reason(l, COMP_REASON_TRANSFORM_3D);
        comp_tile_layer(&comp, lid);
    }

    clock_t t0 = clock();
    comp_calculate_visible_tiles(&comp, 0, 0);
    comp_update_tile_priorities(&comp);
    comp_rasterize_dirty(&comp);
    comp_commit_frame(&comp);
    clock_t t1 = clock();

    uint64_t total_tiles = 0;
    for (int32_t i = 0; i < comp.layer_count; i++) total_tiles += comp.layers[i].tile_count;

    printf("  Layers: %d\n", comp.layer_count);
    printf("  Total tiles: %llu\n", (unsigned long long)total_tiles);
    printf("  Tiles rasterized: %llu\n", (unsigned long long)comp.tiles_rasterized);
    printf("  Memory: %llu bytes\n", (unsigned long long)comp_get_total_memory(&comp));
    printf("  Raster time: %.2f ms\n",
           1000.0 * (double)(t1 - t0) / (double)CLOCKS_PER_SEC);

    comp_destroy(&comp);
}

static void bench_ipc_throughput(void) {
    printf("\n--- IPC Message Throughput ---\n");

    BrowserModel bm;
    bm_init(&bm);
    bm_setup_default(&bm);

    BrowserProcess *browser = bm_get_browser(&bm);
    BrowserProcess *gpu = bm_get_gpu(&bm);
    if (!browser || !gpu) { printf("  SKIP: no browser/gpu\n"); bm_destroy(&bm); return; }

    uint64_t ch = bm_create_ipc_channel(&bm, browser->pid, gpu->pid, IPC_CHANNEL_MOJO);
    IPCChannel *channel = bm_get_channel(&bm, ch);
    if (!channel) { bm_destroy(&bm); return; }

    uint8_t payload[256];
    memset(payload, 0xAB, sizeof(payload));

    clock_t t0 = clock();
    for (int i = 0; i < 1024; i++) {
        bm_send_message(channel, IPC_MSG_NOTIFICATION, payload, sizeof(payload), false);
    }
    clock_t t1 = clock();

    printf("  Messages sent: %llu\n", (unsigned long long)channel->messages_sent);
    printf("  Bytes sent: %llu\n", (unsigned long long)channel->bytes_sent);
    printf("  Time: %.2f ms\n", 1000.0 * (double)(t1 - t0) / (double)CLOCKS_PER_SEC);
    printf("  Throughput: %.1f K msg/s\n",
           1024.0 / ((double)(t1 - t0) / CLOCKS_PER_SEC) / 1000.0);

    IPCMessage recv[128];
    uint32_t count;
    bm_recv_messages(channel, recv, 128, &count);
    printf("  Received: %u messages\n", count);

    bm_destroy(&bm);
}

static void bench_site_isolation(void) {
    printf("\n--- Site Isolation Assignment ---\n");

    BrowserModel bm;
    bm_init(&bm);
    bm_spawn_process(&bm, PROC_BROWSER, "Browser");
    bm_site_isolation_enable(&bm, true);
    bm_site_isolation_per_origin(&bm, true);

    const char *domains[] = {
        "accounts.google.com", "mail.google.com", "docs.google.com",
        "github.com", "evil.com", "bank.example.com",
        "cdn.cloudfront.net", "api.stripe.com", "static.example.org"
    };

    clock_t t0 = clock();
    for (int i = 0; i < 9; i++) {
        Origin o;
        memset(&o, 0, sizeof(o));
        snprintf(o.scheme, 16, "https");
        snprintf(o.host, 128, "%s", domains[i]);
        o.port = 443;
        bm_site_isolation_assign(&bm, &o);
    }
    clock_t t1 = clock();

    printf("  Origins assigned: %d\n", 9);
    printf("  Processes spawned: %u\n", bm.process_count);
    printf("  Time: %.2f ms\n", 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC);

    uint64_t total;
    uint64_t pp[64];
    bm_get_memory_report(&bm, &total, pp);
    printf("  Total memory: %llu MB\n", (unsigned long long)(total / (1024 * 1024)));

    bm_destroy(&bm);
}

static void bench_raf_flood(void) {
    printf("\n--- rAF Flood Test (10K @ 60fps sim) ---\n");
    RenderPipeline rp;
    rp_init(&rp, 60.0);
    g_total_raf_calls = 0;
    uint64_t ts = 0;
    for (int i = 0; i < 10000; i++) {
        rp_request_animation_frame(&rp, count_raf, NULL);
        rp_full_frame(&rp, ts);
        ts += 16667;
    }
    printf("  RAF calls: %llu\n", (unsigned long long)g_total_raf_calls);
    printf("  Frames: %llu\n", (unsigned long long)rp.frame_counter);
    rp_destroy(&rp);
}

int main(void) {
    printf("=== mini-browser-core STRESS BENCHMARK ===\n\n");

    printf("Timestamp: %ld\n", (long)time(NULL));

    bench_event_loop(5000, 16, 1000);
    bench_event_loop(2000, 8, 5000);

    bench_render_pipeline(600, 60.0);
    bench_render_pipeline(300, 120.0);

    bench_compositor();

    bench_ipc_throughput();
    bench_site_isolation();
    bench_raf_flood();

    printf("\n=== Summary ===\n");
    printf("  Total RAF calls: %llu\n", (unsigned long long)g_total_raf_calls);
    printf("  Total frames: %llu (%.1f%% dropped)\n",
           (unsigned long long)g_total_frames,
           g_total_frames ? 100.0 * g_frame_drops / g_total_frames : 0);
    printf("  Total tasks: %llu\n", (unsigned long long)g_total_tasks);
    printf("  Long tasks: %llu\n", (unsigned long long)g_long_tasks);

    printf("\nDone.\n");
    return 0;
}
