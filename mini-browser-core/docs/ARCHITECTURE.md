# mini-browser-core Architecture

## Overall Design

mini-browser-core is a C99 simulation of a modern browser's core subsystems. It models Chromium/Blink/V8 architecture at a conceptual level, providing measurable state machines for each component rather than actual web rendering.

The five modules are designed to work independently but also integrate into a cohesive simulation — see `demos/full_browser_sim.c` for the full integration.

## Module 1: Render Pipeline

```
User Input → Event Loop → requestAnimationFrame
                              ↓
                    Style Recalculation
                              ↓
                          Layout
                              ↓
                          Paint
                              ↓
                        Composite
                              ↓
                       GPU/Display
```

### Frame Lifecycle
- frames are scheduled at vsync intervals (16.67ms @ 60fps)
- each frame has a strict deadline; missed = frame drop
- render_blocking CSS/JS flags gate frame production
- pipeline hooks allow instrumentation at each stage

### Critical Rendering Path
- CSS blocks rendering (external stylesheets must load)
- Sync JS blocks parsing (script tags without async/defer)
- Font blocking delays text rendering until web fonts load
- CR path metrics measure time-to-first-paint (TTFP) equivalents

## Module 2: Event Loop

```
 ┌──────────────────────────────────────┐
 │  Macro Task Queue                    │
 │  setTimeout, I/O, events, postMessage│
 ├──────────────────────────────────────┤
 │  Micro Task Queue                    │
 │  Promise.then, MutationObserver      │
 ├──────────────────────────────────────┤
 │  requestAnimationFrame callbacks     │
 ├──────────────────────────────────────┤
 │  requestIdleCallback                 │
 ├──────────────────────────────────────┤
 │  Style / Layout / Paint              │
 └──────────────────────────────────────┘
```

### Task Processing Order
1. Pick one macro task from queue (oldest ready)
2. Execute the macro task
3. Flush ALL micro tasks (including newly enqueued)
4. If microtask starvation detected (≥128 consecutive), yield
5. Render if needed (rAF frame)
6. Process idle callbacks if idle period available

### Long Task Detection
- Tasks exceeding 50ms are flagged as "long tasks"
- Long tasks contribute to Total Blocking Time (TBT) metric
- Starvation prevention limits consecutive microtask processing

## Module 3: JS Engine Simulation

### Compilation Pipeline
```
Source Code
    ↓
Ignition (Interpreter) ←── always starts here
    ↓ (after ~1K calls)
Sparkplug (Baseline JIT) ←── non-optimizing, fast compilation
    ↓ (after ~10K calls)
TurboFan (Optimizing JIT) ←── speculatively optimized
    ↓ (on deopt)
Back to Ignition
```

### Object Model
- **Smis**: Small integers tagged in pointer bits (no heap allocation)
- **HeapNumber**: Boxed doubles on the heap
- **Hidden Classes**: Shape-based property layout; identical shapes share property offsets
- **Inline Caches**: Monomorphic (fast) → Polymorphic (slower) → Megamorphic (slow)
- **Deoptimization**: Bailout from optimized code when assumptions break

### Garbage Collection
- **Scavenger**: Semi-space copying GC for young generation (frequent, fast)
- **Mark-Compact**: Mark-sweep-compact for old generation (infrequent, thorough)
- **Incremental marking**: GC slices interleaved with JS execution

## Module 4: Compositor

### Layer Tree
```
Root Layer (viewport)
 ├── Paint Layer (document content)
 │    ├── Graphics Layer (CSS transform)
 │    │    └── Tile Grid (256×256)
 │    ├── Video Layer (hardware decoded)
 │    └── IFrame Layer (out-of-process)
 ├── Fixed Position Layer
 └── Overlay Layer (UI chrome)
```

### Compositing Reasons
Layers are promoted to graphics layers (GPU-backed) for:
- 3D transforms
- CSS animations (transform, opacity, filter)
- `<video>` and `<canvas>` elements
- `<iframe>` cross-origin isolation
- will-change CSS property hints
- Fixed/sticky positioning
- Scrollable overflow regions

### Tile Management
- Each layer is subdivided into 256×256 pixel tiles
- Tiles are rasterized on demand (impl-side or main-thread)
- Visible tiles get high priority; off-screen tiles are low priority
- Memory pressure triggers tile eviction (LRU with priority bias)

### Off-Main-Thread Compositing
- Impl-side painting moves raster work off the main thread
- GPU rasterization accelerates tile rendering
- Zero-copy swap chains reduce memory bandwidth

## Module 5: Browser Process Model

### Process Architecture
```
Browser Process (UI, network, policy)
    │
    ├── GPU Process (compositing, WebGL)
    │    └── GPU Decoder (video)
    │
    ├── Renderer Process A (origin: bank.com)
    │    ├── Main thread (JS, layout, paint)
    │    ├── Compositor thread
    │    └── IO thread
    │
    ├── Renderer Process B (origin: evil.com)
    │    └── ... (sandboxed, isolated)
    │
    └── Network Process (all HTTP/WebSocket)
```

### Site Isolation
- Each unique origin gets its own renderer process
- Cross-origin iframes are out-of-process (OOPIF)
- Same-site origins may share a process (configurable)
- Strict isolation prevents Spectre/Meltdown side-channel attacks

### IPC (Inter-Process Communication)
- Mojo-like typed message channels
- Message types: Request, Response, Notification, Stream, Control
- Encrypted and validated channels
- Pending send/receive queues with batching support

## Integration Points

The five modules interact as follows in the full simulation:

1. **Event Loop** drives the execution model
2. RAF callbacks trigger **Render Pipeline** frames
3. **JS Engine** model executes JS tasks queued by the event loop
4. **Compositor** receives painted layers and produces tiles
5. **Browser Model** manages IPC between simulated processes for GPU commands and network requests

See `demos/full_browser_sim.c` for the complete integration.
