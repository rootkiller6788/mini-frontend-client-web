# Layout Engine Demo

## Overview

This demo illustrates the full layout computation pipeline in mini-web-frontend. Starting from a parsed and style-resolved DOM, it builds a layout tree with box-model dimensions, computes flex and block layouts, and positions all elements.

## What This Demo Shows

### 1. Page Structure

The demo uses a realistic page layout:
```
┌─────────────────────────────────────────────────────────┐
│  <header id="top-bar">    (flex, height: 60px)          │
│    <h1>Layout Engine Demo</h1>                          │
├─────────────────────────────────────────────────────────┤
│  <main>    (display: flex)                              │
│  ┌──────────────┬──────────────────────────────────────┐│
│  │ <nav>        │ <section id="content-area">          ││
│  │ (width:200px)│   <article class="post">             ││
│  │ Sidebar      │     <h2>First Article</h2>           ││
│  │              │     <p>Content...</p>                 ││
│  │ Link One     │   </article>                         ││
│  │ Link Two     │   <article class="post">             ││
│  │ Link Three   │     <h2>Second Article</h2>          ││
│  │              │     <p>Content...</p>                 ││
│  │              │     <div class="tags" display:flex>  ││
│  │              │       <span>C</span>                 ││
│  │              │       <span>Layout</span>            ││
│  │              │       <span>Engine</span>            ││
│  │              │     </div>                           ││
│  └──────────────┴──────────────────────────────────────┘│
├─────────────────────────────────────────────────────────┤
│  <footer>    (flex, height: 40px)                       │
│    Footer text                                          │
└─────────────────────────────────────────────────────────┘
```

### 2. Box Model Computation

Each element gets a `BoxModel` with:
```
┌──────────────────────────────────────┐
│  margin                              │
│  ┌────────────────────────────────┐  │
│  │ border                         │  │
│  │ ┌────────────────────────────┐ │  │
│  │ │ padding                    │ │  │
│  │ │ ┌────────────────────────┐ │ │  │
│  │ │ │ content                │ │ │  │
│  │ │ │                        │ │ │  │
│  │ │ └────────────────────────┘ │ │  │
│  │ └────────────────────────────┘ │  │
│  └────────────────────────────────┘  │
└──────────────────────────────────────┘

total_width  = margin_left + border_left + padding_left + content_width
             + padding_right + border_right + margin_right

total_height = margin_top + border_top + padding_top + content_height
             + padding_bottom + border_bottom + margin_bottom
```

The computed styles are parsed for CSS values supporting:
- **px** — absolute pixels
- **em** — relative to root font size (default 16px)
- **rem** — relative to root font size (same as em in this implementation)
- **%** — relative to parent dimension

### 3. Block Layout

Block-level elements (display: block) stack vertically:
```
Parent content box
│
├── Child 1:  pos=(0, 0),             height=100
├── Child 2:  pos=(0, 100),           height=50
├── Child 3:  pos=(0, 150),           height=80
│
└── Parent height = 150 + 80 = 230
```

Each child inherits the parent's content width (minus parent padding). Children are positioned sequentially with `y += child.height`.

Example from the demo — `<article class="post">` elements:
- `First Article` positioned at y=0 within main content area
- `Second Article` positioned at y=total_height_of_first_article

### 4. Flex Layout Simulation

The layout engine simulates CSS flexbox with:

**Flex Direction:**
- `row` — children laid out horizontally (left to right)
- `row-reverse` — children laid out right to left
- `column` — children laid out vertically (top to bottom)
- `column-reverse` — children laid out bottom to top

**Justify Content (main axis distribution):**
| Value | Behavior |
|---|---|
| `flex-start` | Children packed at start |
| `flex-end` | Children packed at end |
| `center` | Children centered |
| `space-between` | First at start, last at end, equal gaps |
| `space-around` | Equal space around each child |
| `space-evenly` | Equal space between all items and edges |

**Align Items (cross axis):**
| Value | Behavior |
|---|---|
| `flex-start` | Children aligned to cross-start |
| `flex-end` | Children aligned to cross-end |
| `center` | Children centered on cross axis |
| `stretch` | Children stretched to fill cross axis |
| `baseline` | Children aligned by text baseline |

Example from the demo — `<div class="tags">`:
```
Display: flex
Direction: row (default)
Justify: flex-start (default)
Children: <span>C</span>  <span>Layout</span>  <span>Engine</span>

Result: tags laid out horizontally with 8px gap
```

### 5. Flex Container Layout Algorithm

```c
void layout_compute_flex(LayoutBox* flex_box, LayoutContext* ctx) {
    // 1. Determine main axis (row → horizontal, column → vertical)
    // 2. Sum all child sizes along main axis
    // 3. Calculate gaps based on justify-content
    //    - space-between: gap = (container_size - total_child_size) / (n - 1)
    //    - space-around:  gap = (container_size - total_child_size) / n
    //    - space-evenly:  gap = (container_size - total_child_size) / (n + 1)
    // 4. Position each child sequentially with gaps
    // 5. Align children on cross axis based on align-items
    // 6. Set container width/height to contain all children
}
```

### 6. Positioning Types

**Static (default):** Follows normal flow. Positioned by parent's layout algorithm.

**Relative:** Not yet implemented as offset from normal position. Currently treated like static (children of relative parents still use normal flow).

**Absolute:** Removed from normal flow. Positioned relative to nearest positioned ancestor using top/right/bottom/left CSS properties.
```c
box->x = parse_css_value(left_val, viewport_width, root_font_size);
box->y = parse_css_value(top_val, viewport_height, root_font_size);
```

**Fixed:** Positioned relative to viewport (same as absolute in this implementation since there's no scrolling).

### 7. Layout Tree vs DOM Tree

The layout tree is a **subset** of the DOM tree:
- Only `ELEMENT` and `DOCUMENT` type DOM nodes become layout boxes
- Text and comment nodes do not have layout boxes
- Elements with `display: none` are excluded from the layout tree
- The layout tree inherits the DOM tree's hierarchy but with different node types

### 8. Recursive Layout Computation

```
layout_compute(root)
  └── layout_compute_box(root)
       ├── If flex → layout_compute_flex(root)
       │    └── For each child: layout_compute_box(child)
       │    └── Distribute children along main axis
       │    └── Align children on cross axis
       │
       └── If block → For each child:
            ├── layout_compute_box(child)  (recursive)
            ├── Position child at content_x, content_y + y_offset
            └── y_offset += child.height

Result: Every box has computed x, y, width, height
```

### 9. Demo Output Analysis

For the header section with `display: flex; align-items: center; height: 60px; padding: 0 20px`:
```
LayoutBox for <header>:
  display=flex
  x=0, y=0
  width=1024 (viewport width)
  height=60
  padding=(0, 20, 0, 20)
  Children:
    <h1>: positioned at content area start, centered vertically
```

For the main content area with `display: flex; min-height: 400px`:
```
LayoutBox for <main>:
  display=flex
  x=0, y=60 (after header)
  width=1024
  height=400
  Children (flex row):
    <nav>:   x=0,   width=230 (200 + 15*2 padding)
    <section>: x=230, width=794 (remaining space)
```

### 10. Limitations of the Layout Engine

- **No text layout**: The engine doesn't measure or wrap text. All text is treated as zero-height content.
- **No flex-grow/shrink/basis**: The flex simulation doesn't support proportional sizing.
- **No margin collapsing**: Adjacent vertical margins don't collapse as per CSS spec.
- **No z-index or stacking context**: All elements are treated as having the same stacking level.
- **No overflow handling**: Content that exceeds its container is not clipped.
- **No table layout**: Tables are treated as block containers.
- **No float**: Float layout is not implemented.

### 11. Running the Demo

```sh
make
bin/layout_example.exe
```

Expected output shows:
1. The parsed HTML and CSS
2. Computed styles for each element
3. The full layout tree with box-model dimensions, positions, and flex properties
4. Box model analysis for key elements
5. Flex child enumeration for the `<main>` container

## Key Data Structures

```c
typedef struct {
    float margin_top, margin_right, margin_bottom, margin_left;
    float border_top, border_right, border_bottom, border_left;
    float padding_top, padding_right, padding_bottom, padding_left;
    float content_width, content_height;
} BoxModel;

typedef struct LayoutBox {
    BoxModel box;                     // Box model dimensions
    float x, y;                       // Computed position
    float width, height;              // Total computed size
    PositionType position;            // static/relative/absolute/fixed
    DisplayType display;              // block/inline/flex/none
    FlexDirection flex_direction;     // row/column
    FlexWrap flex_wrap;              // nowrap/wrap
    JustifyContent justify_content;  // flex-start/center/space-between etc.
    AlignItems align_items;          // stretch/center etc.
    bool laid_out;                    // Whether layout has been computed
    DomNode* dom_node;               // Back-pointer to DOM
    LayoutBox* parent;                // Parent in layout tree
    LayoutBox* first_child;          // First child
    LayoutBox* last_child;           // Last child
    LayoutBox* next_sibling;         // Next sibling
    LayoutBox* prev_sibling;         // Previous sibling
    size_t child_count;              // Number of children
} LayoutBox;
```

## Architecture Flow

```
Computed Style on DOM Node
        │
        v
apply_computed_style_to_layout()
        │
        ├── Parse display value → DisplayType enum
        ├── Parse position value → PositionType enum
        ├── Parse margin/padding (shorthand + longhand)
        ├── Parse width/height (px, em, %, auto)
        └── Set box model fields
        │
        v
layout_compute_box() / layout_compute_flex()
        │
        ├── Position this box
        ├── Compute available space for children
        ├── Recursively layout each child
        └── Update this box's size to contain children
```
