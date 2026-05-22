# mini-mobile-client API Reference

## react_native_bridge.h

### Types

| Type | Description |
|------|-------------|
| `rn_thread_t` | Thread enum: `RN_THREAD_UI`, `RN_THREAD_JS`, `RN_THREAD_BG` |
| `rn_msg_type_t` | Message type: `RN_MSG_CALL`, `RN_MSG_CALLBACK`, `RN_MSG_EVENT`, `RN_MSG_PROMISE` |
| `rn_status_t` | Status: `RN_OK`, `RN_ERR_QUEUE_FULL`, `RN_ERR_MODULE_NF`, `RN_ERR_METHOD_NF`, `RN_ERR_BAD_ARGS`, `RN_ERR_TIMEOUT` |
| `rn_bridge_msg_t` | Bridge message: id, type, target thread, module, method, payload, user_data |
| `rn_callback_fn` | Callback: `void (*)(const char *result, void *user_data)` |
| `rn_native_method_fn` | Native method: `void (*)(const char *args, rn_callback_fn resolve, rn_callback_fn reject)` |
| `rn_module_entry_t` | Module entry: name, handler, loaded flag, active flag |
| `rn_bridge_callback_t` | Callback entry: id, resolve, reject, user_data, created_at, timeout_ms |
| `rn_bridge_t` | Main bridge struct: message queue, module registry, callback manager, batching flag |

### Functions

#### `rn_status_t rn_bridge_init(rn_bridge_t *bridge)`
Initialize the bridge. Allocates the message queue. Returns `RN_OK` on success.

#### `void rn_bridge_destroy(rn_bridge_t *bridge)`
Free all bridge resources.

#### `rn_status_t rn_bridge_register_module(rn_bridge_t *bridge, const char *name, rn_native_method_fn handler)`
Register a native module with its handler function. Duplicate names overwrite the previous registration.

#### `rn_status_t rn_bridge_send_message(rn_bridge_t *bridge, rn_thread_t target, const char *module, const char *method, const char *payload)`
Enqueue a message for a target thread. Module and method identify the native handler.

#### `rn_status_t rn_bridge_send_callback(rn_bridge_t *bridge, int32_t callback_id, const char *result)`
Send a callback result to the JS thread.

#### `rn_status_t rn_bridge_send_promise(rn_bridge_t *bridge, int32_t callback_id, const char *result, bool is_error)`
Resolve or reject a promise. `is_error=true` sends an event-type rejection.

#### `int32_t rn_bridge_register_callback(rn_bridge_t *bridge, rn_callback_fn resolve, rn_callback_fn reject, void *user_data, int64_t timeout_ms)`
Register a callback pair. Returns the callback ID, or -1 on failure.

#### `rn_status_t rn_bridge_flush(rn_bridge_t *bridge, rn_thread_t thread)`
Process all queued messages for the given thread. Invokes registered module handlers.

#### `char *rn_bridge_encode_call(const char *module, const char *method, const char *args)`
Encode a method call into a JSON string. Caller must free the returned string.

#### `rn_status_t rn_bridge_decode_call(const char *encoded, char *module_out, char *method_out, char *args_out, size_t max_len)`
Parse a JSON-encoded call into its components.

#### `int32_t rn_bridge_pending_count(rn_bridge_t *bridge)`
Return the number of unprocessed messages in the queue.

#### `void rn_bridge_set_batching(rn_bridge_t *bridge, bool enable)`
Enable or disable batched message delivery.

---

## native_modules.h

### Types

| Type | Description |
|------|-------------|
| `nm_permission_state_t` | Permission state: GRANTED, DENIED, UNKNOWN, RESTRICT |
| `nm_permission_t` | Permission type: CAMERA, LOCATION, STORAGE, MIC, CONTACTS, NOTIF |
| `nm_lifecycle_t` | Lifecycle: CREATED, STARTING, STARTED, STOPPING, STOPPED, DESTROYED |
| `nm_const_type_t` | Constant type: STRING, INT, DOUBLE, BOOL |
| `nm_constant_t` | Constant: name, type, union value |
| `nm_method_def_t` | Method definition: name, is_async, param_count |
| `nm_method_impl` | Sync method: `int (*)(const char *args, char *result, size_t len, void *ctx)` |
| `nm_async_impl` | Async method: `void (*)(const char *args, char *result, size_t len, void *ctx, void (*done)(const char*, void*), void *cb_ctx)` |
| `nm_module_def_t` | Module definition: name, constants, methods, impls, lifecycle callbacks |
| `nm_module_t` | Module instance: def, state, instance pointer |
| `nm_registry_t` | Module registry: modules array, permission states |

### Functions

#### `int nm_registry_init(nm_registry_t *reg)`
Initialize the registry.

#### `void nm_registry_destroy(nm_registry_t *reg)`
Stop and destroy all modules, free resources.

#### `int nm_register_module(nm_registry_t *reg, const nm_module_def_t *def)`
Register a module definition. Returns 0 on success, -1 on error.

#### `int nm_start_module(nm_registry_t *reg, const char *name)`
Execute the create→start lifecycle. Checks permissions first. Returns -2 if permissions denied, -3 if start failed.

#### `int nm_stop_module(nm_registry_t *reg, const char *name)`
Execute the stop lifecycle.

#### `int nm_call_method(nm_registry_t *reg, const char *module_name, const char *method_name, const char *args, char *result, size_t result_len)`
Synchronously invoke a module method. Returns 0 on success.

#### `int nm_call_method_async(nm_registry_t *reg, ...)`
Asynchronously invoke a module method with a done callback.

#### `int nm_get_constant(nm_registry_t *reg, const char *module_name, const char *const_name, nm_constant_t *out)`
Get a single constant value.

#### `int nm_get_constants(nm_registry_t *reg, const char *module_name, nm_constant_t *out, int max_count)`
Get all constants for a module. Returns the count.

#### `nm_permission_state_t nm_check_permission(nm_registry_t *reg, nm_permission_t perm)`
Check a permission's state.

#### `void nm_set_permission(nm_registry_t *reg, nm_permission_t perm, nm_permission_state_t state)`
Set a permission's state.

#### `nm_module_t *nm_find_module(nm_registry_t *reg, const char *name)`
Find a module by name. Returns NULL if not found.

#### `int nm_module_count(nm_registry_t *reg)`
Return the total number of registered modules.

---

## yoga_layout.h

### Types

| Type | Description |
|------|-------------|
| `yg_direction_t` | Text direction: INHERIT, LTR, RTL |
| `yg_flex_direction_t` | Flex direction: COLUMN, COLUMN_REVERSE, ROW, ROW_REVERSE |
| `yg_justify_t` | Justify content: FLEX_START, CENTER, FLEX_END, SPACE_BETWEEN, SPACE_AROUND, SPACE_EVENLY |
| `yg_align_t` | Alignment: AUTO, FLEX_START, CENTER, FLEX_END, STRETCH, BASELINE, SPACE_BETWEEN, SPACE_AROUND |
| `yg_position_t` | Position type: RELATIVE, ABSOLUTE |
| `yg_unit_t` | Value unit: UNDEFINED, POINT, PERCENT, AUTO |
| `yg_display_t` | Display: FLEX, NONE |
| `yg_overflow_t` | Overflow: VISIBLE, HIDDEN, SCROLL |
| `yg_edge_t` | Edge: LEFT, TOP, RIGHT, BOTTOM, START, END, ALL |
| `yg_value_t` | Typed value: value + unit |
| `yg_size_t` | Size: width, height |
| `yg_rect_t` | Rectangle: x, y, width, height |
| `yg_measure_result_t` | Measure callback result: width, height, measured |
| `yg_measure_fn` | Measure function: `yg_measure_result_t (*)(void *ctx, float w, yg_unit_t wm, float h, yg_unit_t hm)` |
| `yg_node_t` | Layout node: all CSS properties, children, parent, layout result |

### Functions

#### Node Lifecycle
- `yg_node_t *yg_node_create(void)` — Create a new layout node.
- `void yg_node_destroy(yg_node_t *node)` — Free a node.
- `void yg_node_add_child(yg_node_t *parent, yg_node_t *child)` — Add a child.
- `void yg_node_remove_child(yg_node_t *parent, yg_node_t *child)` — Remove a child.
- `int yg_node_child_count(yg_node_t *node)` — Get child count.
- `yg_node_t *yg_node_get_child(yg_node_t *node, int index)` — Get child by index.

#### Container Properties
- `yg_node_set_flex_direction(node, val)`
- `yg_node_set_justify_content(node, val)`
- `yg_node_set_align_items(node, val)`
- `yg_node_set_align_content(node, val)`

#### Item Properties
- `yg_node_set_flex_grow(node, float)`
- `yg_node_set_flex_shrink(node, float)`
- `yg_node_set_flex_basis(node, float, unit)`
- `yg_node_set_align_self(node, val)`

#### Dimensions
- `yg_node_set_width/height(node, float, unit)`
- `yg_node_set_min_width/height(node, float, unit)`
- `yg_node_set_max_width/height(node, float, unit)`

#### Edges
- `yg_node_set_margin/padding(node, edge, float, unit)`
- `yg_node_set_border(node, edge, float)`

#### Positioning
- `yg_node_set_position(node, edge, float, unit)`
- `yg_node_set_position_type(node, val)`

#### Other
- `yg_node_set_display/overflow/aspect_ratio(node, val)`
- `yg_node_set_measure_func(node, fn, ctx)`
- `yg_node_set_user_data/get_user_data(node, data)`

#### Layout
- `void yg_node_calculate_layout(node, available_width, available_height, direction)`
- `yg_rect_t yg_node_get_layout(node)`

#### Value Helpers
- `yg_value_percent(float)` — Create a percent value
- `yg_value_point(float)` — Create a point value
- `yg_value_auto()` — Create an auto value
- `yg_value_undefined()` — Create an undefined value

#### Serialization
- `const char *yg_node_to_json(node, buf, len)` — Serialize node layout to JSON

---

## list_virtualizer.h

### Types

| Type | Description |
|------|-------------|
| `lv_cell_type_t` | Cell type: HEADER, ITEM, FOOTER, SECTION_HEADER, SECTION_FOOTER, SEPARATOR |
| `lv_scroll_state_t` | Scroll state: IDLE, DRAGGING, DECELERATE, ANIMATING |
| `lv_cell_t` | Cell: id, type, section, index, dimensions, y_offset, data, recycled flag |
| `lv_cell_blueprint_t` | Cell template: type, default_height, reuse_id |
| `lv_section_t` | Section: start_index, item_count, header/footer titles |
| `lv_render_cell_fn` | `lv_cell_t *(*)(int index, lv_cell_t *recycled, void *ctx)` |
| `lv_measure_cell_fn` | `float (*)(int index, lv_cell_type_t type, void *ctx)` |
| `lv_on_scroll_fn` | `void (*)(float offset, lv_scroll_state_t state, void *ctx)` |
| `lv_on_refresh_fn` | `void (*)(void *ctx)` |
| `lv_on_end_reached_fn` | `void (*)(void *ctx)` |
| `lv_on_press_fn` | `void (*)(int index, void *ctx)` |
| `lv_controller_t` | Main controller: items, viewport, visible cells, recycle pool, sections, callbacks |

### Functions

#### Lifecycle
- `void lv_init(lv_controller_t *ctrl)` — Initialize the controller
- `void lv_destroy(lv_controller_t *ctrl)` — Clean up resources

#### Data
- `void lv_set_item_count(ctrl, count)` — Set total items, recalculate content height
- `int lv_get_item_count(ctrl)` — Get current item count

#### Cell Types
- `int lv_register_cell_type(ctrl, type, default_height, reuse_id)` — Register a reusable cell type

#### Callbacks
- `lv_set_render_fn(ctrl, fn, ctx)` — Set the cell render callback
- `lv_set_measure_fn(ctrl, fn, ctx)` — Set the cell measurement callback
- `lv_set_scroll_fn(ctrl, fn, ctx)` — Set the scroll event callback
- `lv_set_refresh_fn(ctrl, fn, ctx)` — Set pull-to-refresh callback
- `lv_set_end_reached_fn(ctrl, fn, ctx)` — Set infinite scroll callback
- `lv_set_press_fn(ctrl, fn, ctx)` — Set cell press callback

#### Viewport & Scroll
- `void lv_set_viewport(ctrl, width, height)` — Set the visible viewport dimensions
- `void lv_set_scroll_offset(ctrl, offset)` — Set current scroll position, trigger refresh/end-reached
- `void lv_scroll_to_offset(ctrl, offset, animated)` — Scroll to an absolute offset
- `void lv_scroll_to_index(ctrl, index, animated)` — Scroll to a specific item index

#### Query
- `int lv_get_first_visible(ctrl)` — First visible item index
- `int lv_get_last_visible(ctrl)` — Last visible item index
- `int lv_get_visible_count(ctrl)` — Number of visible items
- `int lv_index_for_point(ctrl, x, y)` — Item index at a given point

#### Refresh & Load More
- `void lv_begin_refresh(ctrl)` / `lv_end_refresh(ctrl)` — Manage pull-to-refresh
- `void lv_set_has_more(ctrl, has_more)` — Enable/disable infinite scroll
- `void lv_load_more_done(ctrl, new_items)` — Signal that new items were loaded

#### Layout
- `void lv_relayout(ctrl)` — Recalculate visible range and recycle cells

#### Cell Recycling
- `lv_cell_t *lv_get_recycled_cell(ctrl, type)` — Get a recycled cell from the pool
- `void lv_recycle_cell(ctrl, cell)` — Return a cell to the recycle pool

#### Sections
- `int lv_add_section(ctrl, start, count, header, footer)` — Add a section definition

---

## push_notif.h

### Types

| Type | Description |
|------|-------------|
| `pn_service_t` | Push service: APNS, FCM, SIM |
| `pn_presentation_t` | Presentation style: ALERT, BADGE, SOUND, BANNER, LIST |
| `pn_app_state_t` | App state: ACTIVE, BACKGROUND, TERMINATED |
| `pn_action_t` | Notification action: id, title, destructive, auth_required, foreground |
| `pn_payload_t` | Notification payload: title, body, badge, sound, data, silent, actions |
| `pn_notification_t` | Full notification: id, payload, deep_link, received_at, read, delivered |
| `pn_manager_t` | Push manager: service, token, callbacks, pending queue |

### Functions

#### Lifecycle
- `void pn_init(pn_manager_t *mgr)` — Initialize the push manager
- `void pn_destroy(pn_manager_t *mgr)` — Reset all state

#### Registration
- `int pn_register_device(mgr, service)` — Register device with a push service, receive token callback

#### Callbacks
- `pn_set_token_callback(mgr, cb, ctx)` — Token received callback
- `pn_set_receive_callback(mgr, cb, ctx)` — Notification received callback
- `pn_set_action_callback(mgr, cb, ctx)` — Action button pressed callback
- `pn_set_deep_link_callback(mgr, cb, ctx)` — Deep link resolved callback
- `pn_set_error_callback(mgr, cb, ctx)` — Error callback

#### Payload Building
- `void pn_build_payload(payload, title, body, data)` — Create a notification payload
- `void pn_set_badge(payload, badge)` — Set badge number
- `void pn_set_silent(payload, silent)` — Mark as silent push
- `void pn_add_action(payload, id, title, destructive, foreground)` — Add an action button
- `void pn_set_deep_link(notif, link)` — Set deep link URL

#### Delivery
- `void pn_deliver_notification(mgr, notif, state)` — Deliver a notification to the app
- `int pn_send_local(mgr, payload, delay_seconds)` — Send a local notification

#### Badge
- `void pn_set_badge_count(mgr, count)` / `int pn_get_badge_count(mgr)` — Manage badge

#### Token & Silent Push
- `void pn_handle_token(mgr, token)` — Handle received device token
- `void pn_handle_silent_push(mgr, data)` — Handle a received silent push

#### Pending Queue
- `int pn_pending_count(mgr)` — Number of undelivered notifications
- `const pn_notification_t *pn_get_pending(mgr, index)` — Get a pending notification
- `void pn_clear_all(mgr)` — Clear all pending notifications and badge

#### Configuration
- `void pn_set_foreground_present(mgr, present)` — Show notifications when app is in foreground
