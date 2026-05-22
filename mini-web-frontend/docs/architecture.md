# mini-web-frontend Architecture

## Overview

mini-web-frontend is a browser rendering engine pipeline implemented in C99. It takes HTML and CSS as input strings and produces a fully resolved DOM tree with computed styles and a layout tree with box-model positioning.

## Pipeline

```
  HTML string                     CSS string
     |                               |
     v                               v
 [html_parser]                  [css_parser]
     |                               |
     v                               v
  DOM Tree                       CSSOM Tree
     |         \               /      |
     |          \             /       |
     |           v           v        |
     |        [style_resolver]        |
     |               |                |
     |               v                |
     |        Computed Styles         |
     |               |                |
     v               v                |
 [layout_engine] <----                |
     |                                |
     v                                |
 Layout Tree                          |
     |                                |
     v                                |
 Render Output (future) ---------------
```

## Modules

### 1. dom_tree (`include/dom_tree.h`, `src/dom_tree.c`)

The core data structure representing the document object model.

**Data structures:**
- `DomNode` — a node in the DOM tree with type (Element, Text, Comment, Document), tag name, attributes, text content, computed styles, and tree pointers (parent, first_child, last_child, next_sibling, prev_sibling)
- `DomAttribute` — name/value pair for element attributes
- `ComputedStyle` — linked list of resolved CSS property/value pairs attached to each element node

**Key operations:**
- Tree construction: `dom_node_create`, `dom_node_append_child`, `dom_node_insert_before`, `dom_node_remove_child`
- Tree traversal: `dom_traverse_preorder`, `dom_traverse_postorder`
- Query: `dom_get_element_by_id`, `dom_get_elements_by_tag_name`, `dom_get_elements_by_class_name`, `dom_query_selector`, `dom_query_selector_all`
- Clone: `dom_node_clone` (shallow or deep)
- Attribute management: `dom_node_set_attribute`, `dom_node_get_attribute`, `dom_node_has_attribute`, `dom_node_remove_attribute`

### 2. html_parser (`include/html_parser.h`, `src/html_parser.c`)

Tokenizes HTML source strings and builds a DOM tree.

**Token types:** open tag, close tag, self-closing tag, text, comment, DOCTYPE.

**Features:**
- Whitespace normalization
- Attribute parsing (quoted and unquoted values)
- Void element detection (br, img, input, etc.)
- Comment stripping
- DOCTYPE recognition
- `html_parse_document` — builds full document with nesting
- `html_parse_fragment` — builds a flat fragment (no nesting closure)

### 3. css_parser (`include/css_parser.h`, `src/css_parser.c`)

Parses CSS source strings into a CSSOM (CSS Object Model) — a collection of rules with selectors and declarations.

**Selector types:** universal (*), tag, class (.), ID (#), attribute ([attr]), pseudo (:pseudo)

**Combinators:** descendant (space), child (>), adjacent sibling (+), general sibling (~)

**Specificity:** Calculated as a 3-component tuple (a, b, c) where:
- a = count of ID selectors
- b = count of class/attribute/pseudo selectors
- c = count of tag selectors

**Features:**
- `!important` flag on declarations
- Comment stripping (/* */)
- @-rule skipping
- Comma-separated selector lists

### 4. style_resolver (`include/style_resolver.h`, `src/style_resolver.c`)

Resolves the CSS cascade against the DOM to produce computed styles for each element.

**Cascade order (lowest to highest priority):**
1. User-agent (default) styles
2. Author styles (normal)
3. Author styles (!important)
4. Inline styles (normal)
5. Inline styles (!important)

**Features:**
- Specificity-based conflict resolution
- Property inheritance (color, font-size, font-family, etc.)
- Default user-agent styles for common HTML elements (h1-h6, p, div, span, a, img, etc.)
- Selector matching against DOM nodes
- `style_resolve_document` — resolves entire document
- `style_resolve_node` — resolves a single subtree with parent style inheritance

### 5. layout_engine (`include/layout_engine.h`, `src/layout_engine.c`)

Computes the visual layout from the DOM and computed styles, producing a layout tree.

**Box model:** Each layout box has margin, border, padding, and content dimensions.

**Display types:** block, inline, inline-block, flex, inline-flex, none

**Positioning:** static, relative, absolute, fixed

**Flex layout simulation:**
- Flex direction: row, row-reverse, column, column-reverse
- Justify content: flex-start, flex-end, center, space-between, space-around, space-evenly
- Align items: flex-start, flex-end, center, stretch
- Automatic distribution of space among flex children

**Layout flow:**
1. Build layout tree recursively from DOM (skip display:none nodes)
2. Apply computed styles to layout boxes
3. Compute box-model dimensions from CSS values (px, em, rem, %)
4. For flex containers, distribute children along main and cross axes
5. For block containers, stack children vertically
6. Position absolute/fixed elements

## Data Flow Example

```
Input HTML: <div id="box" class="container"><p>Hello</p></div>
Input CSS:  .container { padding: 10px; } p { color: red; }

1. html_parse_document → DOM tree:
   [DOCUMENT]
     <div id="box" class="container">
       <p>
         "Hello"

2. css_parse_stylesheet → CSSOM:
   Rule 1: .container → padding: 10px (specificity 0,1,0)
   Rule 2: p → color: red (specificity 0,0,1)

3. style_resolve_document → Computed styles on each element:
   <div>: display=block, padding=10px (from .container)
   <p>:   color=red (from p), display=block (default), font-size=16px (default)

4. layout_compute → Layout tree with positions and sizes
```

## Memory Management

All allocation uses `malloc`/`calloc`/`realloc`/`free`. Each create function has a corresponding free function:
- `dom_node_create` → `dom_node_free` / `dom_node_free_recursive`
- `css_parse_stylesheet` → `css_stylesheet_free`
- `layout_box_create` → `layout_box_free` / `layout_box_free_recursive`

Recursive free functions traverse the entire tree and free all children.

## Limitations

- No actual rendering (pixels) — produces data structures for a renderer to consume
- CSS support covers common selectors and properties but not full spec
- No CSS animation, transitions, or media queries
- Flex layout is a simplified simulation (no flex-grow/shrink/basis)
- No text layout (font metrics, line breaking, word wrapping)
- Single-threaded, synchronous processing
