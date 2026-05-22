# mini-web-frontend — Web前端 (C 语言实现)

A browser rendering engine pipeline written in C99. Parses HTML and CSS, resolves styles via cascade and inheritance, and computes visual layout with box model and flexbox simulation.

## Architecture

```
  HTML string ──→ html_parser ──→ DOM Tree ──┐
                                              ├── style_resolver ──→ Computed Styles
  CSS string  ──→ css_parser  ──→ CSSOM    ──┘                          │
                                                                         v
                                                                  layout_engine
                                                                         │
                                                                         v
                                                                   Layout Tree
```

## Modules

| Module | Header | Source | Purpose |
|---|---|---|---|
| DOM Tree | `include/dom_tree.h` | `src/dom_tree.c` | Core tree structure, traversal, query, manipulation |
| HTML Parser | `include/html_parser.h` | `src/html_parser.c` | Tokenize HTML, build DOM tree |
| CSS Parser | `include/css_parser.h` | `src/css_parser.c` | Parse CSS selectors, declarations, specificity |
| Style Resolver | `include/style_resolver.h` | `src/style_resolver.c` | Cascade resolution, inheritance, computed styles |
| Layout Engine | `include/layout_engine.h` | `src/layout_engine.c` | Box model, flex layout, positioning |

## Building

```sh
make          # Build static library (bin/libminiweb.a) and all examples
make clean    # Remove build artifacts
```

Requirements: GCC (MinGW on Windows, or any GCC on Linux/macOS).

## Quick Start

```c
#include "html_parser.h"
#include "css_parser.h"
#include "dom_tree.h"
#include "style_resolver.h"
#include "layout_engine.h"

int main(void) {
    const char* html = "<div id='app'><h1 class='title'>Hello</h1></div>";
    const char* css  = ".title { color: red; font-size: 24px; }";

    DomNode* doc = html_parse_document(html);
    CssStylesheet* sheet = css_parse_stylesheet(css);
    style_resolve_document(doc, sheet);

    LayoutContext ctx;
    layout_context_init(&ctx, 800, 600);
    LayoutBox* layout = layout_compute(doc, &ctx);

    layout_print_tree(layout, 0);

    layout_context_free(&ctx);
    css_stylesheet_free(sheet);
    dom_node_free_recursive(doc);
    return 0;
}
```

## Examples

```
bin/parse_example.exe    — HTML/CSS parsing and style resolution demo
bin/dom_example.exe      — DOM tree manipulation, queries, traversal
bin/layout_example.exe   — Layout engine with flexbox and box model
```

## Features

- **HTML Parser**: Tokenizes open/close/self-closing tags, attributes (quoted/unquoted), text nodes, comments, DOCTYPE. Handles void elements (br, img, input, etc.) and nested tag tracking.
- **CSS Parser**: Parses tag, class, ID, attribute, pseudo, and universal selectors. Combinators: descendant, child, adjacent sibling, general sibling. `!important` support. Specificity calculation (a,b,c).
- **DOM Tree**: Full tree with parent/child/sibling pointers. `getElementById`, `getElementsByTagName`, `getElementsByClassName`, `querySelector`, `querySelectorAll`. Preorder/postorder traversal. Deep clone.
- **Style Resolver**: Cascade with 5 priority levels. Specificity-based conflict resolution. Property inheritance (color, font, etc.). User-agent default styles for common elements.
- **Layout Engine**: Full box model (margin, border, padding, content). Display: block, inline, flex, none. Positioning: static, relative, absolute, fixed. Flex layout with direction, wrap, justify-content, align-items. CSS value parsing (px, em, rem, %).

## Documentation

- `docs/architecture.md` — Detailed architecture and design decisions
- `docs/api.md` — Complete API reference
- `demos/dom_css_demo/README.md` — DOM and CSS resolution deep-dive
- `demos/layout_demo/README.md` — Layout engine deep-dive

## Conventions

- **C99** standard with `#include <stdbool.h>`
- **snake_case** for functions and variables
- **PascalCase** for types and structs
- **UPPER_SNAKE_CASE** for macros
- `#ifndef` include guards on all headers

## License

Educational project. Use freely.
