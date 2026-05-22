# Developer Guide — mini-frontend-engineering

## Overview

`mini-frontend-engineering` is a C99 implementation of modern frontend build tooling. It provides five core modules covering the entire web development pipeline: bundling, tree shaking, HMR, SSR, and asset processing.

All modules are written in portable C99 with no external dependencies beyond the standard library.

## Project Structure

```
mini-frontend-engineering/
├── bundler_core.h        Bundler header (module resolution, chunk splitting)
├── bundler_core.c        Bundler implementation
├── tree_shaker.h         Tree shaker header (static analysis, dead code)
├── tree_shaker.c         Tree shaker implementation
├── hmr_engine.h          HMR header (WebSocket, hot updates)
├── hmr_engine.c          HMR implementation
├── ssr_model.h           SSR/SSG header (render, hydrate, ISR)
├── ssr_model.c           SSR implementation
├── asset_pipeline.h      Asset pipeline header (images, CSS, sourcemaps)
├── asset_pipeline.c      Asset pipeline implementation
├── example_bundle.c      Bundler example
├── example_shake.c       Tree shaker example
├── example_pipeline.c    Asset pipeline example
├── demo_hmr.c            HMR demonstration
├── demo_ssr.c            SSR/SSG demonstration
├── doc_api.md            API reference
├── doc_guide.md          This guide
├── README.md             Project overview
└── Makefile              Build configuration
```

## Quick Start

### Building

```bash
make all
```

This compiles all source files, examples, and demos.

### Running Examples

```bash
make run-example-bundle    # Module bundler demo
make run-example-shake     # Tree shaking demo
make run-example-pipeline  # Asset pipeline demo
```

### Running Demos

```bash
make run-demo-hmr    # Hot Module Replacement demo
make run-demo-ssr    # SSR/SSG/ISR demo
```

## Module 1: Bundler

### Basic Usage

```c
#include "bundler_core.h"

Bundler b;
bundler_init(&b, "src");
bundler_add_entry(&b, "src/index.js");
bundler_build_dependency_graph(&b);
bundler_split_chunks(&b);
bundler_generate_bundle(&b);
bundler_write_output(&b, "dist");
bundler_print_stats(&b);
```

### Key Features

- **Dependency graph**: Recursively resolves `import` and `require()` calls
- **Module resolution**: Supports relative paths, extension resolution (`.js`, `.ts`, `.json`), and `node_modules`
- **Chunk splitting**: Separates vendor (node_modules) from application code
- **IIFE wrapper**: Each module gets wrapped in an IIFE with a `__require__` function
- **ESM/CJS detection**: Auto-detects module format by scanning for `import`/`export` vs `require`/`module.exports`

### Customizing Resolution

```c
b.resolve.prefer_module = true;  // Use "module" field in package.json
b.resolve.prefer_main = false;   // Use "main" field
// Add custom extensions
strcpy(b.resolve.resolve_extensions[3], ".mjs");
b.resolve.ext_count = 4;
```

## Module 2: Tree Shaker

### Basic Usage

```c
#include "tree_shaker.h"

TreeShaker ts;
shaker_init(&ts);

shaker_add_module(&ts, "math.js", math_source, strlen(math_source));
shaker_add_module(&ts, "index.js", index_source, strlen(index_source));
shaker_set_entry(&ts, "index.js");

// Parse and analyze
ts.modules[0].ast = shaker_parse_source(ts.modules[0].source, ts.modules[0].source_len);
shaker_analyze_exports(&ts, &ts.modules[0]);
shaker_mark_reachable(&ts);
shaker_mark_used_exports(&ts);

// Eliminate and hoist
shaker_eliminate_dead_code(&ts);
shaker_scope_hoisting(&ts);

shaker_print_stats(&ts);
```

### Side Effects

Use `shaker_parse_package_json()` to read the `"sideEffects"` field from `package.json`:

```json
{
  "sideEffects": false
}
```

Modules declared side-effect-free with no used exports are fully removed.

### How It Works

1. **Parse**: Simple AST generation identifies `import`, `export`, `const`, `function`, `class` declarations
2. **Mark Exports**: Each module's exports are identified and initially marked unused
3. **Reachability**: Starting from entry points, mark all reachable modules in the dependency graph
4. **Used Exports**: Track which imports are actually consumed and mark corresponding exports
5. **Eliminate**: Remove unreachable code; side-effect-free modules with 0 used exports become empty
6. **Hoist**: Concatenate remaining modules into a single scope

## Module 3: HMR Engine

### Basic Usage

```c
#include "hmr_engine.h"

HMREngine e;
hmr_init(&e, "ws://localhost:3000/hmr");
hmr_connect(&e);

// Register modules
hmr_register_module(&e, "app.js", hash_app);
hmr_register_module(&e, "utils.js", hash_utils);
hmr_add_dependency(&e, "app.js", "utils.js");

// Mark HMR boundaries
hmr_set_self_accept(&e, "app.js", true);

// Handle incoming update
hmr_handle_update(&e, "{ \"module\": \"utils.js\", \"source\": \"...\" }");
hmr_apply_updates(&e);

hmr_send_status(&e);
```

### HMR Boundary System

An HMR boundary is a module that calls `module.hot.accept()`. In this C implementation, `hmr_set_self_accept()` marks a module as a boundary.

When a file changes:
1. Walk up the dependency tree from the changed module
2. Find the nearest HMR boundary (self-accepting parent)
3. Replace the module in-place at the boundary
4. Preserve application state
5. If no boundary is found, trigger a full page reload

### State Preservation

```c
hmr_save_state_snapshot(&e);    // Save before update
// ... apply updates ...
hmr_restore_state(&e, 0);       // Restore from snapshot 0
```

The engine keeps up to 4 state snapshots in memory.

## Module 4: SSR / SSG / ISR

### Server-Side Rendering

```c
#include "ssr_model.h"

SSREngine e;
ssr_init(&e);

int idx = ssr_register_component(&e, "HomePage");
ssr_set_prop(&e.config.components[idx], "title", "My App");

ssr_register_route(&e, "/", "HomePage");

char html[SSR_HTML_BUFFER_SIZE];
size_t len;
ssr_render_page(&e, "/", html, &len);
printf("%s", html);
```

### Static Site Generation

```c
ssr_static_generate(&e, "dist");
// Generates dist/index.html, dist/about/index.html, etc.
```

### Incremental Static Regeneration

```c
ssr_set_mode(&e, SSR_RENDER_INCREMENTAL);
ssr_set_revalidate(&e.config.routes[0], 60);  // Revalidate every 60 seconds

// On each request:
ssr_isr_check(&e, "/");  // Returns 1 if regenerated
ssr_render_page(&e, "/", html, &len);  // Serves cached or fresh
```

### Streaming SSR

```c
ssr_set_mode(&e, SSR_RENDER_STREAMING);
ssr_streaming_render(&e, "/");

while (ssr_streaming_has_more(&e)) {
    char *chunk = ssr_streaming_next_chunk(&e);
    // Send chunk to client
}
```

### Hydration

```c
// Client side:
ssr_hydrate(&e, server_html, html_len);

// Attach event handlers to server-rendered DOM:
ssr_attach_event_handler(&hydration_node, "click", "handleClick");
ssr_hydrate_node(&hydration_node);
```

## Module 5: Asset Pipeline

### Basic Usage

```c
#include "asset_pipeline.h"

AssetPipeline ap;
ap_init(&ap, "assets", "dist_assets");

ap_add_file(&ap, "assets/styles.scss");
ap_add_file(&ap, "assets/logo.png");

ap_process_all(&ap);
ap_print_report(&ap);
```

### SCSS Compilation

```c
const char *scss = "$primary: blue;\n.btn { color: $primary; &__icon { size: 16px; } }";
char css[65536];
size_t css_len;
ap_compile_sass(&ap, scss, strlen(scss), css, &css_len);
```

Supports:
- Variable resolution (`$variable-name`)
- Nesting with `&` parent selector
- Global and scoped variable definitions

### Autoprefixer

The pipeline comes with pre-configured prefix rules for common properties:

```c
ap_register_prefix(&ap, "grid", true, true, true, false);
// Adds: -webkit-grid, -moz-grid, -ms-grid
```

Pre-registered prefixes: `transform`, `transition`, `animation`, `border-radius`, `box-shadow`, `flex`, `user-select`, `appearance`, `backface-visibility`, `column-count`.

### Content Hashing

```c
char hash[65];
ap_compute_content_hash(data, len, hash);
// Outputs 64-character hex hash string

ap_rename_with_hash(&ap, &file);
// Renames: logo.png -> logo.a1b2c3d4.png
```

### Source Maps

```c
ap_generate_sourcemap(&ap, original, orig_len, processed, proc_len, map, &map_len);

// Inline in output:
ap_sourcemap_base64_inline(&ap, &file);
// Adds: /*# sourceMappingURL=data:application/json;base64,... */
```

## Makefile Targets

```
make all              Build everything
make clean            Remove build artifacts
make run-example-bundle
make run-example-shake
make run-example-pipeline
make run-demo-hmr
make run-demo-ssr
```

## Compatibility

- Standard: C99 (ISO/IEC 9899:1999)
- Compiler: GCC 4.8+, Clang 3.3+, MSVC 2015+
- Platform: POSIX, Windows (cmd.exe or MSYS2)
- Dependencies: libc only (no external libraries)

## Limitations

- The AST parser in `tree_shaker` is simplified and handles common patterns but not all edge cases
- SCSS compiler handles variables and nesting but not mixins, functions, or control directives
- The HMR engine simulates WebSocket communication; real networking would need platform-specific socket APIs
- Image processing functions are stubs (real implementations need libpng/libjpeg or equivalent)
- Streaming SSR uses a simplified chunking strategy

## Architecture Philosophy

Each module is self-contained with its own header and implementation file. There is minimal cross-module coupling. The design prioritizes:

1. **Clarity**: Types and functions map directly to web development concepts
2. **Portability**: Standard C99, no POSIX-specific APIs
3. **Composability**: Each module can be used independently
4. **Simplicity**: Fixed-size arrays with documented limits (all configurable via `#define`)
