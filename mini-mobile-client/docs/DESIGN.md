# mini-mobile-client Design Document

## Architecture Overview

mini-mobile-client implements a modular, layered architecture for native mobile client functionality in pure C99:

```
┌───────────────────────────────────────────────┐
│              Application Layer                 │
│        (demos, examples, user code)            │
├───────────────────────────────────────────────┤
│  Bridge Layer    │   Layout Layer              │
│  ┌─────────────┐ │   ┌──────────────────┐     │
│  │ RN Bridge   │ │   │  Yoga Layout     │     │
│  │ (JS/Native) │ │   │  (Flexbox)       │     │
│  └─────────────┘ │   └──────────────────┘     │
├──────────────────┴────────────────────────────┤
│  Core Services Layer                           │
│  ┌──────────────┐ ┌──────────────┐             │
│  │ Native       │ │ List         │             │
│  │ Modules      │ │ Virtualizer  │             │
│  └──────────────┘ └──────────────┘             │
├───────────────────────────────────────────────┤
│  System Interface Layer                        │
│  ┌──────────────────────────────────┐          │
│  │      Push Notifications          │          │
│  └──────────────────────────────────┘          │
└───────────────────────────────────────────────┘
```

## Module Design

### 1. React Native Bridge (`react_native_bridge.h/.c`)

**Purpose**: Asynchronous message passing bridge between JavaScript and native layers.

**Design Decisions**:
- **Batched Bridge**: Messages are queued in a ring buffer. The `flush` function processes messages per thread, enabling batched delivery similar to React Native's `MessageQueue`.
- **Module Registry**: Native modules register with a name and handler function. Method calls arrive as `module.method` strings.
- **Callback/Promise System**: Each async call can register a callback pair (resolve/reject) with optional timeout. Promises use different message types (`RN_MSG_PROMISE` vs `RN_MSG_EVENT`) for resolution/rejection.
- **Thread Model**: Three threads (UI, JS, Background). The bridge routes messages to the correct thread. Each thread's handler pops its own queued messages.

**Key Data Structures**:
- `rn_bridge_msg_t`: Contains module name, method name, JSON payload, target thread, and message type.
- `rn_bridge_callback_t`: Pairs resolve/reject callbacks with a timeout and creation timestamp.

**Flow**:
```
JS thread                    Bridge                     Native thread
   │                          │                            │
   │── send_message() ────────▶│                            │
   │                          │── [queue message]          │
   │                          │                            │
   │                        flush(UI)──▶                    │
   │                          │      register_callback()    │
   │                          │      dispatch handler()     │
   │                          │◀──────── result ─────────── │
   │◀── send_callback() ──────│                            │
   │                          │                            │
```

### 2. Native Modules (`native_modules.h/.c`)

**Purpose**: Register native capabilities (Camera, Location, Storage, etc.) with lifecycle management and permission control.

**Design Decisions**:
- **Module Definition Pattern**: Each module is described by a `nm_module_def_t` struct containing its name, constants, method definitions, method implementations, lifecycle callbacks (create/start/stop/destroy), and required permissions.
- **Lifecycle State Machine**:
  ```
  CREATED → STARTING → STARTED → STOPPING → STOPPED → DESTROYED
  ```
  Each transition can fail. Permissions are checked before `STARTING`.
- **Thread-Safe Design**: The registry is not inherently thread-safe; callers must synchronize access when using from multiple threads.
- **Sync/Async Methods**: Synchronous methods return immediately. Async methods receive a `done` callback to invoke when the operation completes.
- **Permission Model**: Six permission types (Camera, Location, Storage, Mic, Contacts, Notifications). Each has a state (Granted, Denied, Unknown, Restricted). Modules declare required permissions.

**Constants System**: Module constants are exported as typed values (string, int, double, bool). They can be queried individually or as a batch, analogous to React Native's `constantsToExport`.

### 3. Yoga Layout (`yoga_layout.h/.c`)

**Purpose**: Flexbox-based layout engine similar to Yoga (the layout engine used by React Native).

**Design Decisions**:
- **CSS Flexbox Subset**: Implements flexDirection, justifyContent, alignItems, flexGrow, flexShrink, flexBasis, alignSelf, positioning, margins, padding, borders, min/max dimensions.
- **Absolute Positioning**: Absolutely positioned children are removed from the flex flow and positioned using explicit coordinates (left, top, right, bottom).
- **Percentage Values**: Percentage values are resolved relative to the parent's size at layout time.
- **Text Measurement**: Custom measurement functions can be attached to leaf nodes. This enables measuring text (which requires platform-specific font metrics) without coupling the layout engine to a rendering backend.
- **Layout Pass**: Single-pass layout calculation. The algorithm:
  1. Calculate available space from parent constraints and own dimensions.
  2. Subtract padding and border to get content area.
  3. Separate absolute-positioned children.
  4. Distribute space among flex children based on flexGrow.
  5. Position children (column: top-to-bottom; row: left-to-right).
  6. Apply justifyContent offset.
  7. Clamp to min/max dimensions.

**Limitations**: Does not implement flex-wrap, gap, or percent-of-remaining-space (stretch is supported). This is intentionally simplified to 130+ lines.

### 4. List Virtualizer (`list_virtualizer.h/.c`)

**Purpose**: High-performance virtualized list similar to FlatList/VirtualizedList in React Native.

**Design Decisions**:
- **Windowed Rendering**: Only cells visible within the viewport + a small buffer are "rendered" (measured and positioned). Non-visible cells don't consume memory for render data.
- **Cell Recycling**: When cells scroll off-screen, they're returned to a recycle pool. When new cells come into view, recycled cells are reused with new data. This minimizes allocation.
- **Cell Type System**: Six cell types: Header, Item, Footer, Section Header, Section Footer, Separator. Each type has a default height and reuse identifier. The recycle pool matches by cell type.
- **Infinite Scroll**: When the user scrolls within `end_reached_threshold` pixels of the end, the `end_reached_fn` callback fires. The application loads more data and calls `lv_load_more_done`.
- **Pull-to-Refresh**: When the user pulls down past `refresh_threshold`, the refresh callback fires. The application refreshes data and calls `lv_end_refresh`.
- **Scroll-to-Index**: Calculates the offset of a given item by accumulating heights (using the measure callback when available, or estimated heights). Then scrolls to that offset.
- **Sections**: Supports section-based layouts with start index, item count, and header/footer titles.

**Memory Model**:
```
[0..N items total]
     │
     ▼
  Virtualizer keeps visible cells (~15-20 in memory)
     │
     ├── Scrolled off-screen → Recycle pool (max 32)
     ├── Scrolled into view  ← Recycle pool (reuse)
     └── New cells → Allocate directly
```

### 5. Push Notifications (`push_notif.h/.c`)

**Purpose**: Manage push notifications including registration, payload building, delivery, and user interaction.

**Design Decisions**:
- **Service Abstraction**: Supports APNs, FCM, and a simulated service (`PN_SERVICE_SIM`). The service type affects token format but the API is uniform.
- **Notification Actions**: Interactive notifications support action buttons with configurable behavior:
  - `destructive`: Styled as a destructive action (red).
  - `foreground`: Whether the action brings the app to foreground.
  - `authentication_required`: Whether the action requires authentication.
- **Foreground/Background Handling**: When the app is in the foreground, notifications can either be shown immediately or queued as pending (controlled by `foreground_present` flag).
- **Silent Push**: `content_available=true` pushes don't display a UI. They invoke the receive callback directly for data sync operations. The payload body/title may be empty.
- **Deep Linking**: Each notification can carry a deep link URL. When the user taps the notification, the deep link callback fires with the URL.
- **Badge Management**: The badge count is maintained independently and can be set programmatically or through notification payloads.
- **Local Notifications**: The framework supports sending local (in-app) notifications with an optional delay.

**Callback Architecture**:
```
Register Device → token_callback(token)
Receive Push    → receive_callback(notification, app_state)
User Action     → action_callback(action_id, notification)
Notification Tap→ deep_link_callback(deep_link_url)
Error           → error_callback(code, message)
```

## Data Flow

### Typical Application Initialization

```
1. rn_bridge_init()        — Create JS↔Native bridge
2. nm_registry_init()      — Initialize native module registry
3. Register modules         — Camera, Location, Storage, etc.
4. nm_start_module()       — Start each module (check permissions → create → start)
5. yg_node_create()        — Build layout tree
6. yg_node_calculate_layout() — Compute positions/sizes
7. lv_init()               — Initialize list controller
8. lv_set_item_count()     — Set data source
9. lv_relayout()           — Compute visible cells, render
10. pn_init()              — Initialize push notifications
11. pn_register_device()   — Get push token
```

### User Interaction Flow

```
User scrolls list
       │
       ▼
lv_set_scroll_offset()
       │
       ├── Trigger pull-to-refresh (scroll < -threshold)
       ├── Trigger load-more    (scroll near end + has_more)
       └── Trigger scroll callback
              │
              ▼
       lv_relayout() — recompute visible range, recycle cells
```

## Memory Management

- **Ownership**: The bridge owns its message queue. The registry owns module instances. Layout nodes are individually owned; destroying a parent does NOT recursively destroy children (callers must manage child lifecycle).
- **Allocation**: `calloc`/`free` for dynamic memory. Cell recycling in the list virtualizer avoids frequent allocation.
- **Strings**: Fixed-size char arrays throughout (no dynamic string allocation for names/payloads). The one exception is `rn_bridge_encode_call` which allocates a string the caller must free.

## Portability

The code uses only:
- Standard C99 headers (`stdlib.h`, `string.h`, `stdio.h`, `math.h`, `time.h`)
- No platform-specific APIs

To integrate with a real platform (iOS, Android, embedded), implement platform-specific hooks for:
- `rn_bridge_flush`: Thread dispatching
- `yg_measure_fn`: Platform text measurement
- `pn_register_device`: Actual APNs/FCM registration
- `lv_render_cell_fn`: Platform UI rendering

## Limitations & Future Work

1. **Yoga Layout**: Missing flex-wrap, gap, RTL text direction resolution, and percent-of-remaining-space.
2. **List Virtualizer**: No support for varying column counts, horizontal lists, or sticky headers.
3. **Push Notifications**: No encryption, no JWT token for APNs, no actual network layer.
4. **Thread Safety**: No mutexes in the bridge or registry. Single-threaded usage assumed.
5. **Error Handling**: Return codes are basic. No error propagation chains.
