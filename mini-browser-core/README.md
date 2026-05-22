# mini-browser-core — 浏览器核心 (C 语言实现)

A C99 simulation of modern browser core architecture: rendering pipeline, event loop, JS engine (V8 model), compositor, and multi-process browser model.

## Architecture Overview

```
 mini-browser-core/
 ├── include/                  # Public headers
 │   ├── render_pipeline.h     # Rendering pipeline stages
 │   ├── event_loop.h          # Event loop with micro/macro queues
 │   ├── js_engine_sim.h       # V8 engine simulation
 │   ├── composite_layer.h     # Compositor & layer tree
 │   └── concurrency_browser.h # Multi-process browser model
 ├── src/                      # Implementation (C99)
 │   ├── render_pipeline.c     # Frame lifecycle, CR path
 │   ├── event_loop.c          # Task queues, rAF, rIC
 │   ├── js_engine_sim.c       # JIT tiers, shapes, IC, GC
 │   ├── composite_layer.c     # Tiles, raster, impl-side
 │   └── concurrency_browser.c # IPC, site isolation
 ├── examples/                 # Usage examples
 │   ├── demo01_frame_loop.c
 │   ├── demo02_js_engine.c
 │   └── demo03_browser_process.c
 ├── demos/                    # Full demonstrations
 │   ├── full_browser_sim.c    # All 5 modules integrated
 │   └── stress_benchmark.c    # Performance benchmarks
 ├── docs/                     # Documentation
 │   ├── ARCHITECTURE.md
 │   └── API_REFERENCE.md
 ├── Makefile
 └── README.md
```

## Module Summary

| Module | File | Key Concepts |
|--------|------|-------------|
| Render Pipeline | `render_pipeline.h/.c` | Style→Layout→Paint→Composite, vsync, rAF, render-blocking |
| Event Loop | `event_loop.h/.c` | Macro/micro task queues, rIC, long task detection |
| JS Engine Sim | `js_engine_sim.h/.c` | Ignition→Sparkplug→TurboFan, hidden classes, IC, Scavenger+MC |
| Composite Layer | `composite_layer.h/.c` | Layer tree, tiles, compositing reasons, impl-side painting |
| Browser Model | `concurrency_browser.h/.c` | Process-per-origin, site isolation, Mojo-like IPC |

## Build & Run

### Prerequisites
- C99 compiler (GCC ≥5, Clang ≥3.5, MSVC 2015+)
- GNU Make (or nmake on Windows)

### Build
```sh
make          # Build all examples and demos
make examples # Build examples only
make demos    # Build demos only
make debug    # Build with debug symbols
make clean    # Clean build artifacts
```

### Run
```sh
./build/demo01_frame_loop
./build/demo02_js_engine
./build/demo03_browser_process
./build/full_browser_sim
./build/stress_benchmark
```

## Key Design Decisions

- **C99 only**: No C11/C17 features. Portable to embedded and legacy toolchains.
- **No external dependencies**: Standard library only.
- **Fixed-size static arrays**: Predictable memory footprint, no fragmentation.
- **Simulation model**: Models browser internals with measurable state transitions, not actual rendering.
- **Single-threaded simulation**: All modules run cooperatively — reflective of the main-thread-constrained reality.

## Rendering Pipeline

Critical rendering path simulation:

1. **Style Recalculation**: CSS cascade resolution simulation
2. **Layout**: Box tree construction with containing block awareness
3. **Paint**: Display list generation, layer creation
4. **Composite**: Layer merging, GPU texture upload simulation

Frame lifecycle with vsync budget (16.67ms @ 60fps), deadline checking, and frame drop detection.

## Event Loop

- **Macro tasks**: setTimeout, setInterval, I/O events, message passing
- **Micro tasks**: Promise.then/catch, MutationObserver, queueMicrotask
- **requestIdleCallback**: Low-priority work with deadline awareness
- **Long task detection**: 50ms threshold, starvation prevention

## JS Engine Simulation

V8 conceptual model:
- **JIT compilation tiers**: Ignition (interpreter) → Sparkplug (baseline) → TurboFan (optimizing)
- **Hidden classes (shapes)**: Map-based property layout for fast property access
- **Inline caches**: Monomorphic→Polymorphic→Megamorphic state transitions
- **Object representation**: Smis (tagged integers), HeapNumber, tagged pointers
- **Garbage collection**: Semi-space Scavenger for young generation, Mark-Compact for old

## Compositor

- Layer tree with parent-child hierarchy
- Compositing reasons: transform, opacity, video, iframe, WebGL, will-change
- Tile-based rasterization (256×256 tiles)
- Impl-side painting support
- Tile eviction with priority-based LRU

## Browser Process Model

- **Process types**: Browser, GPU, Renderer, Network, Utility
- **Site isolation**: Process-per-origin with cross-origin separation
- **IPC**: Mojo-like message channels with typed messages
- **Sandboxing**: Per-process sandbox flags

## License

MIT
