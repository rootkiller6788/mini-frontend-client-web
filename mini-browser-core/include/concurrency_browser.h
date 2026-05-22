#ifndef CONCURRENCY_BROWSER_H
#define CONCURRENCY_BROWSER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_PROCESSES       64
#define MAX_IPC_CHANNELS    128
#define MAX_SITE_ISOLATIONS 32
#define MAX_ORIGINS         32
#define IPC_MSG_MAX_SIZE    4096

typedef enum {
    PROC_BROWSER        = 0,
    PROC_GPU            = 1,
    PROC_RENDERER       = 2,
    PROC_NETWORK        = 3,
    PROC_UTILITY        = 4,
    PROC_PLUGIN         = 5,
    PROC_EXTENSION      = 6,
    PROC_GPU_DECODER    = 7,
    PROC_AUDIO          = 8
} ProcessType;

typedef enum {
    IPC_MSG_REQUEST      = 0,
    IPC_MSG_RESPONSE     = 1,
    IPC_MSG_NOTIFICATION = 2,
    IPC_MSG_STREAM       = 3,
    IPC_MSG_CONTROL      = 4
} IPCMessageType;

typedef enum {
    IPC_CHANNEL_MOJO     = 0,
    IPC_CHANNEL_PIPE     = 1,
    IPC_CHANNEL_SHARED   = 2,
    IPC_CHANNEL_SOCKET   = 3
} IPCChannelType;

typedef enum {
    IPC_DISCONNECTED     = 0,
    IPC_CONNECTING       = 1,
    IPC_CONNECTED        = 2,
    IPC_ERROR            = 3
} IPCChannelState;

typedef enum {
    SITE_SAME_ORIGIN     = 0,
    SITE_SAME_SITE       = 1,
    SITE_CROSS_ORIGIN    = 2,
    SITE_ISOLATED        = 3
} SiteRelation;

typedef struct {
    char     scheme[16];
    char     host[128];
    uint16_t port;
} Origin;

typedef struct {
    uint64_t    msg_id;
    IPCMessageType type;
    uint32_t    source_pid;
    uint32_t    target_pid;
    uint32_t    payload_size;
    uint8_t     payload[IPC_MSG_MAX_SIZE];
    uint64_t    timestamp_us;
    bool        expects_reply;
    bool        is_urgent;
} IPCMessage;

typedef struct {
    uint64_t         channel_id;
    IPCChannelType   type;
    IPCChannelState  state;
    uint32_t         source_pid;
    uint32_t         target_pid;
    uint64_t         messages_sent;
    uint64_t         messages_received;
    uint64_t         bytes_sent;
    uint64_t         bytes_received;
    bool             is_encrypted;
    bool             is_validated;
    IPCMessage       pending_sends[64];
    uint32_t         pending_count;
    IPCMessage       pending_recvs[64];
    uint32_t         recv_count;
} IPCChannel;

typedef struct {
    uint32_t     pid;
    ProcessType  type;
    char         name[64];
    bool         is_sandboxed;
    bool         is_running;
    bool         is_main;
    bool         is_privileged;
    uint64_t     memory_bytes;
    uint64_t     cpu_usage_us;
    uint64_t     start_time_ms;
    uint64_t     crash_count;
    uint32_t     parent_pid;
    uint32_t     child_pids[16];
    uint32_t     child_count;
    Origin       bound_origin;
    bool         site_isolated;
    uint32_t     ipc_channels[32];
    uint32_t     ipc_channel_count;
} BrowserProcess;

typedef struct {
    Origin       origins[MAX_ORIGINS];
    uint32_t     origin_count;
    uint32_t     process_per_origin[MAX_ORIGINS];
    bool         site_isolation_enabled;
    bool         per_origin_mode;
    uint32_t     isolation_groups[MAX_SITE_ISOLATIONS][MAX_ORIGINS];
    uint32_t     group_sizes[MAX_SITE_ISOLATIONS];
    uint32_t     group_count;
} SiteIsolation;

typedef struct {
    BrowserProcess    processes[MAX_PROCESSES];
    uint32_t          process_count;
    uint32_t          next_pid;

    IPCChannel        channels[MAX_IPC_CHANNELS];
    uint32_t          channel_count;
    uint64_t          next_channel_id;

    SiteIsolation     isolation;
    Origin            trusted_origins[16];
    uint32_t          trusted_count;

    bool              sandbox_enabled;
    bool              mojo_enabled;
    bool              service_worker_support;
    bool              out_of_process_iframes;
    uint32_t          max_renderer_per_site;

    uint64_t          total_ipc_messages;
    uint64_t          total_memory_all;
    char              user_data_dir[512];
} BrowserModel;

void bm_init(BrowserModel *bm);
void bm_destroy(BrowserModel *bm);

uint32_t bm_spawn_process(BrowserModel *bm, ProcessType type, const char *name);
uint32_t bm_spawn_renderer(BrowserModel *bm, const Origin *origin);
void bm_terminate_process(BrowserModel *bm, uint32_t pid);
BrowserProcess *bm_get_process(BrowserModel *bm, uint32_t pid);
BrowserProcess *bm_get_browser(const BrowserModel *bm);
BrowserProcess *bm_get_gpu(const BrowserModel *bm);
BrowserProcess *bm_get_network(const BrowserModel *bm);

uint64_t bm_create_ipc_channel(BrowserModel *bm, uint32_t src, uint32_t dst,
                               IPCChannelType type);
IPCChannel *bm_get_channel(BrowserModel *bm, uint64_t ch_id);
int bm_send_message(IPCChannel *ch, IPCMessageType type,
                    const uint8_t *payload, uint32_t size, bool expects_reply);
int bm_recv_messages(IPCChannel *ch, IPCMessage *out, uint32_t max_count,
                     uint32_t *received);
void bm_close_channel(IPCChannel *ch);

void bm_site_isolation_register(BrowserModel *bm, const Origin *origin);
uint32_t bm_site_isolation_get_pid(const BrowserModel *bm, const Origin *origin);
uint32_t bm_site_isolation_assign(BrowserModel *bm, const Origin *origin);
bool bm_site_isolation_is_same_origin(const Origin *a, const Origin *b);
SiteRelation bm_site_isolation_relation(const Origin *a, const Origin *b);
void bm_site_isolation_enable(BrowserModel *bm, bool enable);
void bm_site_isolation_per_origin(BrowserModel *bm, bool enable);

void bm_enable_sandbox(BrowserModel *bm, bool enable);
void bm_enable_mojo(BrowserModel *bm, bool enable);
void bm_set_trusted_origin(BrowserModel *bm, const Origin *origin);

void bm_setup_default(BrowserModel *bm);
void bm_get_memory_report(const BrowserModel *bm, uint64_t *total,
                          uint64_t per_process[]);
const char *bm_process_type_name(ProcessType type);
void bm_dump_processes(const BrowserModel *bm);

#endif
