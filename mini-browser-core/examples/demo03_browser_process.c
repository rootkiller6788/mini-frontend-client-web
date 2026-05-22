#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "concurrency_browser.h"
#include "composite_layer.h"

static void send_test_message(IPCChannel *ch, const char *text) {
    bm_send_message(ch, IPC_MSG_REQUEST,
                    (const uint8_t *)text, (uint32_t)strlen(text) + 1, false);
}

int main(void) {
    printf("=== mini-browser-core Demo 3: Browser Process Model + Compositing ===\n\n");

    BrowserModel bm;
    bm_init(&bm);

    printf("--- Spawning default processes ---\n");
    bm_setup_default(&bm);

    Origin site_a, site_b;
    memset(&site_a, 0, sizeof(site_a));
    snprintf(site_a.scheme, 16, "https");
    snprintf(site_a.host, 128, "bank.example.com");
    site_a.port = 443;

    memset(&site_b, 0, sizeof(site_b));
    snprintf(site_b.scheme, 16, "https");
    snprintf(site_b.host, 128, "evil.example.com");
    site_b.port = 443;

    printf("\n--- Site Isolation ---\n");
    uint32_t pid_a = bm_site_isolation_assign(&bm, &site_a);
    uint32_t pid_b = bm_site_isolation_assign(&bm, &site_b);
    printf("  bank.example.com -> PID %u\n", pid_a);
    printf("  evil.example.com -> PID %u\n", pid_b);

    SiteRelation rel = bm_site_isolation_relation(&site_a, &site_b);
    printf("  bank <-> evil relation: %d (0=sameOrigin 1=sameSite 2=crossOrigin)\n", rel);

    printf("\n--- IPC Channels ---\n");
    uint64_t ch1 = bm_create_ipc_channel(&bm, pid_a, 100, IPC_CHANNEL_MOJO);
    uint64_t ch2 = bm_create_ipc_channel(&bm, pid_a, 101, IPC_CHANNEL_MOJO);
    printf("  Created channels: %llu, %llu\n",
           (unsigned long long)ch1, (unsigned long long)ch2);

    IPCChannel *c1 = bm_get_channel(&bm, ch1);
    if (c1) send_test_message(c1, "Hello from Renderer A");

    IPCMessage received[8];
    uint32_t count;
    bm_recv_messages(c1, received, 8, &count);
    printf("  Received %u messages on channel %llu\n", count, (unsigned long long)ch1);
    for (uint32_t i = 0; i < count; i++) {
        printf("    msg[%u] type=%d from=%u to=%u payload='%s'\n",
               i, received[i].type, received[i].source_pid,
               received[i].target_pid, received[i].payload);
    }

    printf("\n--- Compositor with layers ---\n");
    Compositor comp;
    comp_init(&comp, 1920, 1080, 2.0f);

    int32_t vid_layer = comp_create_layer(&comp, LAYER_VIDEO, comp.root_layer_idx);
    comp_set_layer_bounds(comp_get_layer(&comp, vid_layer), 100, 100, 640, 360);
    comp_add_compositing_reason(comp_get_layer(&comp, vid_layer), COMP_REASON_VIDEO);

    int32_t ifr_layer = comp_create_layer(&comp, LAYER_IFRAME, comp.root_layer_idx);
    comp_set_layer_bounds(comp_get_layer(&comp, ifr_layer), 800, 100, 400, 600);
    comp_add_compositing_reason(comp_get_layer(&comp, ifr_layer), COMP_REASON_IFRAME);

    int32_t anim = comp_create_layer(&comp, LAYER_GRAPHICS, comp.root_layer_idx);
    comp_set_layer_bounds(comp_get_layer(&comp, anim), 0, 0, 300, 300);
    comp_set_layer_opacity(comp_get_layer(&comp, anim), 0.75f);
    comp_add_compositing_reason(comp_get_layer(&comp, anim), COMP_REASON_OPACITY_ANIM);

    comp_tile_layer(&comp, vid_layer);
    comp_tile_layer(&comp, ifr_layer);
    comp_tile_layer(&comp, anim);

    comp_calculate_visible_tiles(&comp, 150, 50);
    comp_update_tile_priorities(&comp);
    comp_rasterize_dirty(&comp);
    comp_commit_frame(&comp);

    printf("\n--- Layer Tree ---\n");
    comp_dump_layer_tree(&comp, comp.root_layer_idx, 0);
    printf("  Total compositor memory: %llu bytes\n",
           (unsigned long long)comp_get_total_memory(&comp));

    printf("\n--- Full process dump ---\n");
    bm_dump_processes(&bm);

    uint64_t total_mem;
    uint64_t per_proc[64];
    bm_get_memory_report(&bm, &total_mem, per_proc);
    printf("\n  Memory report: total=%llu MB\n",
           (unsigned long long)(total_mem / (1024 * 1024)));

    comp_destroy(&comp);
    bm_destroy(&bm);

    printf("\nDone.\n");
    return 0;
}
