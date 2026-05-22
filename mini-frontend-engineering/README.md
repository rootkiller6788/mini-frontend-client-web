# mini-frontend-engineering — 前端工程化 (C 语言实现)

Modern frontend engineering toolchain implemented in portable C99.

## Modules

| Module | Header | Description |
|--------|--------|-------------|
| Bundler | `bundler_core.h` | Module dependency graph, resolution (node_modules, path aliases), chunk splitting (vendor/app), code splitting (dynamic import), IIFE-wrapped output bundles |
| Tree Shaker | `tree_shaker.h` | Static analysis of import/export, mark used exports (entry -> reachable), dead code elimination, side effect detection (package.json), scope hoisting (module concatenation) |
| HMR | `hmr_engine.h` | WebSocket connection, module dependency graph, HMR boundary (self-accept), in-place module replacement, state preservation, fallback to full reload |
| SSR/SSG | `ssr_model.h` | Server-side render (renderToString), hydration (attach event handlers), streaming SSR, Static Site Generation (build-time render), ISR (incremental static regeneration) |
| Asset Pipeline | `asset_pipeline.h` | Image optimization (resize/format/webp), CSS preprocessing (SCSS variables & nesting), PostCSS autoprefixer simulation, source maps (base64 inline), content hashing for cache busting |

## File Manifest

```
mini-frontend-engineering/
├── bundler_core.h        Module bundler: dependency graph, resolution,
│                           chunk splitting, IIFE wrapper
├── tree_shaker.h         Tree shaking: static analysis, dead code
│                           elimination, side effects, scope hoisting
├── hmr_engine.h          Hot Module Replacement: WebSocket, HMR boundary,
│                           state preservation, full reload fallback
├── ssr_model.h           SSR/SSG: renderToString, hydration, streaming
│                           SSR, SSG build, ISR revalidation
├── asset_pipeline.h      Asset pipeline: image optimization, SCSS
│                           compilation, autoprefixer, sourcemaps, hashing
├── bundler_core.c        Bundler implementation (200+ LOC)
├── tree_shaker.c         Tree shaker implementation (200+ LOC)
├── hmr_engine.c          HMR engine implementation (200+ LOC)
├── ssr_model.c           SSR/SSG implementation (200+ LOC)
├── asset_pipeline.c      Asset pipeline implementation (300+ LOC)
├── example_bundle.c      Bundler usage example
├── example_shake.c       Tree shaker usage example
├── example_pipeline.c    Asset pipeline usage example
├── demo_hmr.c            HMR full demonstration (250+ LOC)
├── demo_ssr.c            SSR/SSG/ISR full demonstration (250+ LOC)
├── doc_api.md            Complete API reference
├── doc_guide.md          Developer guide with usage examples
├── README.md             This file
└── Makefile              Build configuration
```

## Quick Start

```bash
make all                  # Build all libraries, examples, and demos
make run-example-bundle   # Run bundler example
make run-example-shake    # Run tree shaker example
make run-example-pipeline # Run asset pipeline example
make run-demo-hmr         # Run HMR demo
make run-demo-ssr         # Run SSR/SSG demo
make clean                # Clean build artifacts
```

## Requirements

- C99-compatible compiler (GCC, Clang, MSVC)
- Standard C library only (no external dependencies)
- POSIX or Windows (cmd.exe) environment

## License

MIT
