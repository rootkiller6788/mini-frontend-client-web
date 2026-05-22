# mini-browser-core API Reference

## Render Pipeline (`render_pipeline.h`)

### Lifecycle
```c
void rp_init(RenderPipeline *rp, double fps);
void rp_destroy(RenderPipeline *rp);
```

### Configuration
```c
void rp_set_frame_budget_us(RenderPipeline *rp, uint64_t budget_us);
void rp_set_vsync(RenderPipeline *rp, bool enabled);
```

### Frame Management
```c
void rp_request_animation_frame(RenderPipeline *rp, FrameCallback cb, void *user_data);
void rp_cancel_animation_frame(RenderPipeline *rp, FrameCallback cb);
void rp_begin_frame(RenderPipeline *rp, uint64_t timestamp_us);
FrameInfo rp_get_frame_info(const RenderPipeline *rp);
```

### Pipeline Execution
```c
void rp_full_frame(RenderPipeline *rp, uint64_t timestamp_us);
void rp_perform_style_recalc(RenderPipeline *rp);
void rp_perform_layout(RenderPipeline *rp);
void rp_perform_paint(RenderPipeline *rp);
void rp_perform_composite(RenderPipeline *rp);
```

### Hooks
```c
void rp_register_hook(RenderPipeline *rp, PipelineStage stage, PipelineHook hook, void *user_data);
void rp_disable_stage(RenderPipeline *rp, PipelineStage stage);
void rp_enable_stage(RenderPipeline *rp, PipelineStage stage);
```

### Render Blocking
```c
void rp_add_render_blocking(RenderPipeline *rp, RenderBlocking block);
void rp_remove_render_blocking(RenderPipeline *rp, RenderBlocking block);
```

### Metrics
```c
double rp_frame_duration_ms(const FrameInfo *fi);
double rp_get_utilization(const RenderPipeline *rp);
const char *rp_stage_name(PipelineStage stage);
```

## Event Loop (`event_loop.h`)

### Lifecycle
```c
void el_init(EventLoop *el);
void el_destroy(EventLoop *el);
```

### Task Scheduling
```c
uint64_t el_schedule_task(EventLoop *el, TaskType type, TaskFn fn,
                          void *user_data, uint64_t delay_ms);
uint64_t el_schedule_recurring(EventLoop *el, TaskType type, TaskFn fn,
                               void *user_data, uint64_t interval_ms);
void el_cancel_task(EventLoop *el, uint64_t task_id);
void el_cancel_all_type(EventLoop *el, TaskType type);
```

### Microtasks
```c
uint64_t el_queue_microtask(EventLoop *el, MicroTaskType type,
                            TaskFn fn, void *user_data);
void el_enqueue_microtask(EventLoop *el, TaskFn fn, void *user_data);
void el_flush_microtasks(EventLoop *el);
```

### Idle Callbacks
```c
uint64_t el_request_idle_callback(EventLoop *el, IdleCallbackFn fn, void *user_data);
void el_cancel_idle(EventLoop *el, uint64_t id);
```

### Execution
```c
void el_run(EventLoop *el);
void el_tick(EventLoop *el, uint64_t now_ms);
void el_process_one_macro(EventLoop *el, uint64_t now_ms);
```

### Time
```c
void el_set_time(EventLoop *el, uint64_t now_ms);
uint64_t el_now(const EventLoop *el);
```

### Configuration
```c
void el_enable_long_task_detection(EventLoop *el, bool enable);
void el_enable_starvation_prevention(EventLoop *el, bool enable);
void el_set_micro_starvation_limit(EventLoop *el, uint32_t limit);
```

### Metrics
```c
TaskMetrics el_get_last_metrics(const EventLoop *el);
uint64_t el_get_long_task_count(const EventLoop *el);
uint64_t el_get_total_processed(const EventLoop *el);
bool el_is_empty(const EventLoop *el);
void el_dump_state(const EventLoop *el);
```

## JS Engine Simulation (`js_engine_sim.h`)

### Isolate
```c
void js_isolate_init(Isolate *iso, uint64_t id);
void js_isolate_destroy(Isolate *iso);
void js_isolate_set_main_thread(Isolate *iso, bool main);
```

### Value Creation
```c
JSValue js_make_smi(Isolate *iso, int32_t val);
JSValue js_make_heap_number(Isolate *iso, double val);
JSValue js_make_string(Isolate *iso, const char *str);
JSValue js_make_boolean(Isolate *iso, bool val);
JSValue js_make_undefined(void);
JSValue js_make_null(void);
```

### Hidden Classes & Objects
```c
HiddenClass *js_create_shape(Isolate *iso, const char *prop_keys[], uint32_t count);
HiddenClass *js_get_shape(Isolate *iso, uint64_t hash);

JSObject *js_create_object(Isolate *iso, HiddenClass *shape);
void js_set_property(Isolate *iso, JSObject *obj, uint32_t offset, JSValue val);
JSValue js_get_property(const JSObject *obj, uint32_t offset);
```

### JIT Compilation
```c
void js_jit_on_call(Isolate *iso, uint64_t func_hash);
JITTier js_jit_determine_tier(const JITState *js);
void js_jit_compile(Isolate *iso, uint64_t func_hash, JITTier tier);
void js_jit_deoptimize(Isolate *iso, JITTier from, uint64_t reason);
const char *js_jit_tier_name(JITTier tier);
```

### Inline Caching
```c
InlineCache *js_ic_get_or_create(Isolate *iso, const char *name);
ICState js_ic_update(InlineCache *ic, HiddenClass *observed_shape, bool hit);
void js_ic_invalidate(InlineCache *ic);
```

### Garbage Collection
```c
void js_gc_trigger_scavenge(Isolate *iso);
void js_gc_trigger_mark_compact(Isolate *iso);
void js_gc_trigger_full(Isolate *iso);
void js_gc_step_incremental(Isolate *iso);
bool js_gc_is_in_progress(const GCState *gc);
const char *js_gc_phase_name(GCPhase phase);
```

### Diagnostics
```c
const MemoryStats *js_get_memory_stats(const Isolate *iso);
void js_dump_shapes(const Isolate *iso);
void js_dump_ic_state(const Isolate *iso);
```

## Compositor (`composite_layer.h`)

### Lifecycle
```c
void comp_init(Compositor *c, int32_t vp_w, int32_t vp_h, float dsf);
void comp_destroy(Compositor *c);
```

### Layer Management
```c
int32_t comp_create_layer(Compositor *c, LayerType type, int32_t parent_id);
Layer *comp_get_layer(Compositor *c, int32_t idx);
void comp_remove_layer(Compositor *c, int32_t idx);
```

### Layer Properties
```c
void comp_set_layer_transform(Layer *l, const float matrix[16]);
void comp_set_layer_opacity(Layer *l, float opacity);
void comp_set_layer_bounds(Layer *l, float x, float y, float w, float h);
void comp_set_layer_position(Layer *l, float x, float y);
```

### Compositing Reasons
```c
void comp_add_compositing_reason(Layer *l, CompositingReason reason);
bool comp_has_reason(const Layer *l, CompositingReason reason);
void comp_evaluate_compositing(Layer *l);
```

### Viewport
```c
void comp_set_viewport(Compositor *c, int32_t w, int32_t h, float dsf);
```

### Tile Management
```c
void comp_tile_layer(Compositor *c, int32_t layer_idx);
void comp_rasterize_tile(Layer *l, uint32_t tile_id);
void comp_rasterize_dirty(Compositor *c);
void comp_calculate_visible_tiles(Compositor *c, float scroll_x, float scroll_y);
void comp_update_tile_priorities(Compositor *c);
void comp_evict_tiles(Compositor *c, uint64_t max_bytes);
```

### Frame Commit
```c
void comp_commit_frame(Compositor *c);
```

### Configuration
```c
void comp_set_impl_side_painting(Compositor *c, bool enabled);
void comp_set_gpu_raster(Compositor *c, bool enabled);
void comp_set_zero_copy(Compositor *c, bool enabled);
```

### Query
```c
bool comp_is_composited_anim(const Layer *l);
uint64_t comp_get_total_memory(const Compositor *c);
const char *comp_layer_type_name(LayerType type);
const char *comp_reason_name(CompositingReason reason);
void comp_dump_layer_tree(const Compositor *c, int32_t idx, int32_t depth);
```

## Browser Process Model (`concurrency_browser.h`)

### Lifecycle
```c
void bm_init(BrowserModel *bm);
void bm_destroy(BrowserModel *bm);
```

### Process Management
```c
uint32_t bm_spawn_process(BrowserModel *bm, ProcessType type, const char *name);
uint32_t bm_spawn_renderer(BrowserModel *bm, const Origin *origin);
void bm_terminate_process(BrowserModel *bm, uint32_t pid);
BrowserProcess *bm_get_process(BrowserModel *bm, uint32_t pid);
BrowserProcess *bm_get_browser(const BrowserModel *bm);
BrowserProcess *bm_get_gpu(const BrowserModel *bm);
BrowserProcess *bm_get_network(const BrowserModel *bm);
```

### IPC Channels
```c
uint64_t bm_create_ipc_channel(BrowserModel *bm, uint32_t src, uint32_t dst,
                               IPCChannelType type);
IPCChannel *bm_get_channel(BrowserModel *bm, uint64_t ch_id);
int bm_send_message(IPCChannel *ch, IPCMessageType type,
                    const uint8_t *payload, uint32_t size, bool expects_reply);
int bm_recv_messages(IPCChannel *ch, IPCMessage *out, uint32_t max_count,
                     uint32_t *received);
void bm_close_channel(IPCChannel *ch);
```

### Site Isolation
```c
void bm_site_isolation_register(BrowserModel *bm, const Origin *origin);
uint32_t bm_site_isolation_get_pid(const BrowserModel *bm, const Origin *origin);
uint32_t bm_site_isolation_assign(BrowserModel *bm, const Origin *origin);
bool bm_site_isolation_is_same_origin(const Origin *a, const Origin *b);
SiteRelation bm_site_isolation_relation(const Origin *a, const Origin *b);
void bm_site_isolation_enable(BrowserModel *bm, bool enable);
void bm_site_isolation_per_origin(BrowserModel *bm, bool enable);
```

### Configuration
```c
void bm_enable_sandbox(BrowserModel *bm, bool enable);
void bm_enable_mojo(BrowserModel *bm, bool enable);
void bm_set_trusted_origin(BrowserModel *bm, const Origin *origin);
void bm_setup_default(BrowserModel *bm);
```

### Diagnostics
```c
void bm_get_memory_report(const BrowserModel *bm, uint64_t *total,
                          uint64_t per_process[]);
const char *bm_process_type_name(ProcessType type);
void bm_dump_processes(const BrowserModel *bm);
```
