# mini-frontend-framework — 前端框架 (C 语言实现)

A C99 implementation of modern frontend architecture patterns: Virtual DOM, React-style Hooks, Component Model, SPA Router, and Reactive Signals. Zero external dependencies.

## Modules

| Module | Header | Description |
|--------|--------|-------------|
| Virtual DOM | `virtual_dom.h` | VNode types, createElement, diff algorithm, keyed reconciliation, DOM patching, batch updates |
| Hooks | `react_hooks.h` | useState, useEffect, useRef, useMemo, useContext, Fiber-like linked list |
| Component Model | `component_model.h` | Functional components, lifecycle (mount/update/unmount), Context API, Error Boundary, Suspense |
| SPA Router | `spa_router.h` | pushState/replaceState, route config, nested routes, guards, lazy loading, hash fallback |
| Reactive System | `reactive_system.h` | Signals, Effects, Computed values, batch updates, memoization, auto-tracking |

## Building

```bash
make all
```

Or manually:

```bash
gcc -std=c99 -Wall -Wextra -O2 -c virtual_dom.c react_hooks.c component_model.c spa_router.c reactive_system.c
gcc -std=c99 -Wall -Wextra -O2 example_basic.c *.o -o example_basic
gcc -std=c99 -Wall -Wextra -O2 example_todo.c *.o -o example_todo
gcc -std=c99 -Wall -Wextra -O2 example_router.c *.o -o example_router
gcc -std=c99 -Wall -Wextra -O2 demo_perf.c *.o -o demo_perf
gcc -std=c99 -Wall -Wextra -O2 demo_reactive.c *.o -o demo_reactive
```

## Quick Start

```c
#include "virtual_dom.h"
#include "react_hooks.h"
#include "component_model.h"

VNode* hello_component(Component* comp, VNode* props) {
    (void)props;
    HookState* hs = comp->hook_state;
    hooks_begin_render(hs);
    hooks_end_render(hs);

    VNode* div = vdom_create_element("div");
    VNode* h1  = vdom_create_element("h1");
    vdom_set_text(h1, "Hello from C99!");
    vdom_append_child(div, h1);
    return div;
}

int main(void) {
    Component* app = comp_create("App", hello_component);
    comp_mount(app, NULL);

    VNode* tree = comp_render(app);
    vdom_debug_print(tree, 0);

    vdom_free_recursive(tree);
    comp_free(app);
    return 0;
}
```

## Running Examples

```bash
./example_basic     # Basic VDOM, hooks, component demo
./example_todo      # Todo app with diff & reconciliation
./example_router    # SPA router with guards & nested routes
./demo_perf         # Performance benchmarks
./demo_reactive     # Signals, effects, computed values
```

## Architecture

```
Application Code
    │
    ├── Component Model (lifecycle, context, error boundaries)
    ├── SPA Router (routing, history, guards)
    └── Reactive System (signals, effects, computed)
            │
            ├── React Hooks (useState, useEffect, useRef, useMemo)
            │
            └── Virtual DOM (VNode tree, diff, patch, batch)
                    │
                    └── C99 Standard Library
```

## Key Features

- **Virtual DOM**: Full VNode tree with keyed reconciliation (`O(n)` diff)
- **Hooks**: useState, useEffect (with cleanup), useRef, useMemo, useContext
- **Components**: Lifecycle callbacks (mount/update/unmount), props system
- **Router**: History API + hash fallback, URL params, nested routes, guards
- **Reactivity**: Signals with auto-tracking Effects, Computed values, batch updates
- **Memory**: Manual allocation with explicit free, no GC overhead
- **Portability**: Pure C99, no platform-specific APIs, static compilation

## File Structure

```
mini-frontend-framework/
├── virtual_dom.h       # VDOM types and function declarations
├── virtual_dom.c       # VDOM implementation
├── react_hooks.h       # Hooks types and function declarations
├── react_hooks.c       # Hooks implementation
├── component_model.h   # Component types and function declarations
├── component_model.c   # Component implementation
├── spa_router.h        # Router types and function declarations
├── spa_router.c        # Router implementation
├── reactive_system.h   # Reactive types and function declarations
├── reactive_system.c   # Reactive implementation
├── example_basic.c     # Basic example
├── example_todo.c      # Todo app example
├── example_router.c    # Router example
├── demo_perf.c         # Performance demo
├── demo_reactive.c     # Reactive signals demo
├── API_REFERENCE.md    # Complete API documentation
├── DESIGN.md           # Architecture and design decisions
├── README.md           # This file
└── Makefile            # Build configuration
```

## License

MIT
