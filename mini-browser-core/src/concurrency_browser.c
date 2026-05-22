#include "concurrency_browser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bm_init(BrowserModel *bm) {
    memset(bm, 0, sizeof(*bm));
    bm->next_pid = 100;
    bm->next_channel_id = 1;
    bm->sandbox_enabled = true;
    bm->mojo_enabled = true;
    bm->service_worker_support = true;
    bm->out_of_process_iframes = true;
    bm->max_renderer_per_site = 4;
    bm->isolation.site_isolation_enabled = true;
    bm->isolation.per_origin_mode = false;
}

void bm_destroy(BrowserModel *bm) {
    memset(bm, 0, sizeof(*bm));
}

uint32_t bm_spawn_process(BrowserModel *bm, ProcessType type, const char *name) {
    if (bm->process_count >= MAX_PROCESSES) return 0;
    uint32_t pid = bm->next_pid++;
    BrowserProcess *p = &bm->processes[bm->process_count++];
    memset(p, 0, sizeof(*p));
    p->pid = pid;
    p->type = type;
    snprintf(p->name, sizeof(p->name), "%s", name ? name : "");
    p->is_running = true;
    p->is_sandboxed = bm->sandbox_enabled && (type == PROC_RENDERER || type == PROC_GPU_DECODER);
    p->is_privileged = (type == PROC_BROWSER);
    p->is_main = (type == PROC_BROWSER);
    p->start_time_ms = 0;
    p->crash_count = 0;
    p->site_isolated = false;
    if (type == PROC_BROWSER) {
        p->memory_bytes = 200 * 1024 * 1024;
    } else if (type == PROC_GPU) {
        p->memory_bytes = 300 * 1024 * 1024;
    } else if (type == PROC_RENDERER) {
        p->memory_bytes = 150 * 1024 * 1024;
    } else if (type == PROC_NETWORK) {
        p->memory_bytes = 80 * 1024 * 1024;
    } else {
        p->memory_bytes = 50 * 1024 * 1024;
    }
    return pid;
}

uint32_t bm_spawn_renderer(BrowserModel *bm, const Origin *origin) {
    uint32_t pid = bm_spawn_process(bm, PROC_RENDERER, "Renderer");
    if (!pid) return 0;
    BrowserProcess *p = bm_get_process(bm, pid);
    if (p && origin) {
        memcpy(&p->bound_origin, origin, sizeof(Origin));
        if (bm->isolation.site_isolation_enabled) {
            p->site_isolated = true;
        }
    }
    return pid;
}

void bm_terminate_process(BrowserModel *bm, uint32_t pid) {
    BrowserProcess *p = bm_get_process(bm, pid);
    if (!p) return;
    p->is_running = false;
}

BrowserProcess *bm_get_process(BrowserModel *bm, uint32_t pid) {
    for (uint32_t i = 0; i < bm->process_count; i++) {
        if (bm->processes[i].pid == pid) return &bm->processes[i];
    }
    return NULL;
}

BrowserProcess *bm_get_browser(const BrowserModel *bm) {
    for (uint32_t i = 0; i < bm->process_count; i++) {
        if (bm->processes[i].type == PROC_BROWSER) return (BrowserProcess *)&bm->processes[i];
    }
    return NULL;
}

BrowserProcess *bm_get_gpu(const BrowserModel *bm) {
    for (uint32_t i = 0; i < bm->process_count; i++) {
        if (bm->processes[i].type == PROC_GPU) return (BrowserProcess *)&bm->processes[i];
    }
    return NULL;
}

BrowserProcess *bm_get_network(const BrowserModel *bm) {
    for (uint32_t i = 0; i < bm->process_count; i++) {
        if (bm->processes[i].type == PROC_NETWORK) return (BrowserProcess *)&bm->processes[i];
    }
    return NULL;
}

uint64_t bm_create_ipc_channel(BrowserModel *bm, uint32_t src, uint32_t dst,
                               IPCChannelType type) {
    if (bm->channel_count >= MAX_IPC_CHANNELS) return 0;
    uint64_t ch_id = bm->next_channel_id++;
    IPCChannel *ch = &bm->channels[bm->channel_count++];
    memset(ch, 0, sizeof(*ch));
    ch->channel_id = ch_id;
    ch->type = type;
    ch->state = IPC_CONNECTING;
    ch->source_pid = src;
    ch->target_pid = dst;
    ch->is_encrypted = true;
    ch->is_validated = true;
    ch->state = IPC_CONNECTED;

    BrowserProcess *sp = bm_get_process(bm, src);
    BrowserProcess *tp = bm_get_process(bm, dst);
    if (sp && sp->ipc_channel_count < 32) sp->ipc_channels[sp->ipc_channel_count++] = (uint32_t)ch_id;
    if (tp && tp->ipc_channel_count < 32) tp->ipc_channels[tp->ipc_channel_count++] = (uint32_t)ch_id;

    return ch_id;
}

IPCChannel *bm_get_channel(BrowserModel *bm, uint64_t ch_id) {
    for (uint32_t i = 0; i < bm->channel_count; i++) {
        if (bm->channels[i].channel_id == ch_id) return &bm->channels[i];
    }
    return NULL;
}

int bm_send_message(IPCChannel *ch, IPCMessageType type,
                    const uint8_t *payload, uint32_t size, bool expects_reply) {
    if (!ch || ch->state != IPC_CONNECTED) return -1;
    if (size > IPC_MSG_MAX_SIZE) return -2;
    if (!payload && size > 0) return -3;

    IPCMessage msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_id = ch->messages_sent + 1;
    msg.type = type;
    msg.source_pid = ch->source_pid;
    msg.target_pid = ch->target_pid;
    msg.payload_size = size;
    if (payload && size > 0) memcpy(msg.payload, payload, size);
    msg.expects_reply = expects_reply;
    msg.timestamp_us = (uint64_t)(ch->messages_sent * 100);

    if (ch->recv_count < 64) {
        ch->pending_recvs[ch->recv_count++] = msg;
    }
    ch->messages_sent++;
    ch->bytes_sent += size;
    return 0;
}

int bm_recv_messages(IPCChannel *ch, IPCMessage *out, uint32_t max_count,
                     uint32_t *received) {
    if (!ch || !out || !received) return -1;
    *received = 0;
    if (ch->recv_count == 0) return 0;
    uint32_t count = ch->recv_count < max_count ? ch->recv_count : max_count;
    for (uint32_t i = 0; i < count; i++) {
        out[i] = ch->pending_recvs[i];
    }
    ch->recv_count -= count;
    if (ch->recv_count > 0)
        memmove(ch->pending_recvs, ch->pending_recvs + count,
                ch->recv_count * sizeof(IPCMessage));
    ch->messages_received += count;
    for (uint32_t i = 0; i < count; i++) ch->bytes_received += out[i].payload_size;
    *received = count;
    return 0;
}

void bm_close_channel(IPCChannel *ch) {
    if (!ch) return;
    ch->state = IPC_DISCONNECTED;
}

void bm_site_isolation_register(BrowserModel *bm, const Origin *origin) {
    if (bm->isolation.origin_count >= MAX_ORIGINS) return;
    for (uint32_t i = 0; i < bm->isolation.origin_count; i++) {
        if (bm_site_isolation_is_same_origin(&bm->isolation.origins[i], origin)) return;
    }
    memcpy(&bm->isolation.origins[bm->isolation.origin_count], origin, sizeof(Origin));
    bm->isolation.process_per_origin[bm->isolation.origin_count] = 0;
    bm->isolation.origin_count++;
}

uint32_t bm_site_isolation_get_pid(const BrowserModel *bm, const Origin *origin) {
    for (uint32_t i = 0; i < bm->isolation.origin_count; i++) {
        if (bm_site_isolation_is_same_origin(&bm->isolation.origins[i], origin))
            return bm->isolation.process_per_origin[i];
    }
    return 0;
}

uint32_t bm_site_isolation_assign(BrowserModel *bm, const Origin *origin) {
    uint32_t pid = bm_site_isolation_get_pid(bm, origin);
    if (pid) return pid;
    pid = bm_spawn_process(bm, PROC_RENDERER, "SiteRenderer");
    bm_site_isolation_register(bm, origin);
    for (uint32_t i = 0; i < bm->isolation.origin_count; i++) {
        if (bm_site_isolation_is_same_origin(&bm->isolation.origins[i], origin)) {
            bm->isolation.process_per_origin[i] = pid;
            break;
        }
    }
    return pid;
}

bool bm_site_isolation_is_same_origin(const Origin *a, const Origin *b) {
    if (!a || !b) return false;
    if (strncmp(a->scheme, b->scheme, 16) != 0) return false;
    if (strncmp(a->host, b->host, 128) != 0) return false;
    if (a->port != b->port) return false;
    return true;
}

SiteRelation bm_site_isolation_relation(const Origin *a, const Origin *b) {
    if (bm_site_isolation_is_same_origin(a, b)) return SITE_SAME_ORIGIN;
    if (strncmp(a->scheme, b->scheme, 16) == 0 &&
        strstr(a->host, b->host + (b->host[0] == '.' ? 0 : 0))) return SITE_SAME_SITE;
    return SITE_CROSS_ORIGIN;
}

void bm_site_isolation_enable(BrowserModel *bm, bool enable) {
    bm->isolation.site_isolation_enabled = enable;
}

void bm_site_isolation_per_origin(BrowserModel *bm, bool enable) {
    bm->isolation.per_origin_mode = enable;
}

void bm_enable_sandbox(BrowserModel *bm, bool enable) {
    bm->sandbox_enabled = enable;
}

void bm_enable_mojo(BrowserModel *bm, bool enable) {
    bm->mojo_enabled = enable;
}

void bm_set_trusted_origin(BrowserModel *bm, const Origin *origin) {
    if (bm->trusted_count >= 16) return;
    memcpy(&bm->trusted_origins[bm->trusted_count++], origin, sizeof(Origin));
}

void bm_setup_default(BrowserModel *bm) {
    bm_spawn_process(bm, PROC_BROWSER, "Browser");
    bm_spawn_process(bm, PROC_GPU, "GPU");
    bm_spawn_process(bm, PROC_NETWORK, "Network");

    uint32_t browser_pid = 100, gpu_pid = 101, net_pid = 102;
    bm_create_ipc_channel(bm, browser_pid, gpu_pid, IPC_CHANNEL_MOJO);
    bm_create_ipc_channel(bm, browser_pid, net_pid, IPC_CHANNEL_MOJO);

    Origin example;
    memset(&example, 0, sizeof(example));
    snprintf(example.scheme, sizeof(example.scheme), "https");
    snprintf(example.host, sizeof(example.host), "example.com");
    example.port = 443;
    bm_site_isolation_assign(bm, &example);
}

void bm_get_memory_report(const BrowserModel *bm, uint64_t *total,
                          uint64_t per_process[]) {
    *total = 0;
    for (uint32_t i = 0; i < bm->process_count; i++) {
        per_process[i] = bm->processes[i].memory_bytes;
        *total += bm->processes[i].memory_bytes;
    }
}

const char *bm_process_type_name(ProcessType type) {
    static const char *names[] = { "Browser","GPU","Renderer","Network",
                                   "Utility","Plugin","Extension","GpuDecoder","Audio" };
    return (type <= 8) ? names[type] : "?";
}

void bm_dump_processes(const BrowserModel *bm) {
    printf("[BrowserModel] %u processes:\n", bm->process_count);
    uint64_t total_mem = 0;
    for (uint32_t i = 0; i < bm->process_count; i++) {
        const BrowserProcess *p = &bm->processes[i];
        printf("  PID=%u %s name=%s sandbox=%d siteIsolated=%d mem=%lluMB crash=%llu\n",
               p->pid, bm_process_type_name(p->type), p->name,
               p->is_sandboxed, p->site_isolated,
               (unsigned long long)(p->memory_bytes / (1024 * 1024)),
               (unsigned long long)p->crash_count);
        total_mem += p->memory_bytes;
    }
    printf("  Channels: %u | Total memory: %llu MB\n", bm->channel_count,
           (unsigned long long)(total_mem / (1024 * 1024)));
}
