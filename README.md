# Mini Frontend Client Web

A collection of **from-scratch, zero-dependency C implementations** of frontend web technologies, browser engines, UI frameworks, and mobile client architectures. Each module models real frontend system behavior — from DOM rendering to virtual DOM diffing, bundler tree-shaking, state management, and native mobile bridges.

## Modules

| Module | Topics | Key References |
|--------|--------|----------------|
| [mini-web-frontend](mini-web-frontend/) | HTML parser, CSS parser + cascade, DOM tree, layout engine (flexbox sim), font rendering model | Chromium Blink, W3C Specs |
| [mini-browser-base](mini-browser-base/) | HTTP cache (etag/last-modified), cookies/storage (local/session), event system, fetch/XHR model | MDN, WhatWG Fetch |
| [mini-browser-core](mini-browser-core/) | Rendering pipeline (style→layout→paint→composite), event loop (macro/micro tasks), V8 isolate sim, GC from browser | Chrome V8, Chromium Docs |
| [mini-frontend-engineering](mini-frontend-engineering/) | Module bundler (webpack sim), tree-shaking (dead code elimination), HMR (hot module replacement), SSR/SSG model | Webpack, Vite, Next.js |
| [mini-frontend-framework](mini-frontend-framework/) | Virtual DOM (create/diff/patch), React-like hooks (useState/useEffect), component model, JSX-like transpiler sim, SPA routing | React, Vue, SolidJS |
| [mini-state-data](mini-state-data/) | Redux store (actions/reducers/middleware), MobX observable, Zustand-like stores, React Query cache sim, immutable update | Redux, Zustand, React Query |
| [mini-mobile-client](mini-mobile-client/) | React Native bridge (JS↔Native), native module registry, Flexbox Yoga sim, list virtualization, push notification model | React Native, Flutter |
| [mini-ui-ux-hci](mini-ui-ux-hci/) | Design tokens system, component library catalog, accessibility tree (ARIA), gesture/click model, dark mode theming | Material Design, Ant Design |

## Design Philosophy

- **Zero external dependencies** — pure C (C99/C11), only `libc` and `libm`
- **Self-contained modules** — each directory has its own `Makefile`, `include/`, `src/`, `examples/`, `demos/`, `tests/`
- **Frontend simulation in user-space** — educational models of browser engines, frontend frameworks, and mobile clients
- **Theory-to-code mapping** — every module includes `docs/` with reference-alignment notes
- **Practical demos** — virtual DOM engine, CSS layout engine, module bundler, React Native bridge simulator, and more

## Building

Each module is standalone. Navigate to a module directory and run:

```bash
cd mini-frontend-framework
make all    # build everything
make test   # run tests
```

Requires **GCC** and **GNU Make**.

## Project Structure

```
mini-frontend-client-web/
├── mini-web-frontend/           # Web Frontend Basics
├── mini-browser-base/           # Browser Base Capabilities
├── mini-browser-core/           # Browser Core Engine
├── mini-frontend-engineering/   # Frontend Engineering
├── mini-frontend-framework/     # Frontend Framework
├── mini-state-data/             # State Management & Data
├── mini-mobile-client/          # Mobile Client
└── mini-ui-ux-hci/              # UI/UX & HCI
```

## License

MIT
