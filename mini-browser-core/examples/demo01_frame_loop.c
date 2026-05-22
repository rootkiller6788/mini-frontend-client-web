#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "render_pipeline.h"
#include "event_loop.h"
#include "composite_layer.h"

static EventLoop el;
static RenderPipeline rp;
static Compositor comp;

static void raf_callback(uint64_t ts_ms, void *data) {
    printf("  [RAF] frame callback @ %llu ms\n", (unsigned long long)ts_ms);
    (void)data;
}

static void demo_task(void *data) {
    printf("  [Task] %s executes\n", (const char *)data);
}

static void demo_microtask(void *data) {
    printf("  [Micro] %s completes\n", (const char *)data);
}

static void stage_hook(PipelineStage stage, void *data) {
    printf("  [Pipeline] %s stage\n", rp_stage_name(stage));
    (void)data;
}

int main(void) {
    printf("=== mini-browser-core Demo 1: Frame Pipeline + Event Loop ===\n\n");

    el_init(&el);
    rp_init(&rp, 60.0);
    comp_init(&comp, 1920, 1080, 1.0f);

    rp_register_hook(&rp, STYLE_RECALC, stage_hook, NULL);
    rp_register_hook(&rp, LAYOUT_BUILD, stage_hook, NULL);

    rp_request_animation_frame(&rp, raf_callback, NULL);

    el_schedule_task(&el, TASK_SETTIMEOUT, demo_task, "setTimeout A", 50);
    el_schedule_task(&el, TASK_EVENT_CLICK, demo_task, "click handler", 10);
    el_queue_microtask(&el, MICRO_PROMISE_THEN, demo_microtask, "Promise.then");
    el_queue_microtask(&el, MICRO_MUTATION_OBSERVER, demo_microtask, "MutationObserver");

    uint64_t sim_time_ms = 0;
    for (int tick = 0; tick < 19; tick++) {
        sim_time_ms += 16;
        el_set_time(&el, sim_time_ms);
        el_tick(&el, sim_time_ms);
    }

    printf("\n--- Running a full frame ---\n");
    rp_full_frame(&rp, sim_time_ms * 1000);
    FrameInfo fi = rp_get_frame_info(&rp);
    printf("  Frame %llu: state=%d missed=%d duration=%.2fms\n",
           (unsigned long long)fi.frame_id, fi.state, fi.missed_deadline,
           rp_frame_duration_ms(&fi));

    printf("\n--- Compositor layer tree ---\n");
    comp_dump_layer_tree(&comp, comp.root_layer_idx, 0);

    printf("\n--- Task metrics ---\n");
    TaskMetrics tm = el_get_last_metrics(&el);
    printf("  Last task: type=%d elapsed=%llums long=%d\n",
           tm.task_type, (unsigned long long)tm.elapsed_ms, tm.is_long_task);
    printf("  Long tasks: %llu / %llu total\n",
           (unsigned long long)el_get_long_task_count(&el),
           (unsigned long long)el_get_total_processed(&el));

    el_dump_state(&el);

    comp_destroy(&comp);
    rp_destroy(&rp);
    el_destroy(&el);

    printf("\nDone.\n");
    return 0;
}
