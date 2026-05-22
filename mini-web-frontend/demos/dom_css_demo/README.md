# DOM & CSS Resolution Demo

## Overview

This demo illustrates the full DOM tree construction and CSS style resolution pipeline in mini-web-frontend. It walks through parsing HTML into a DOM tree, parsing CSS into a CSSOM ruleset, resolving styles via cascade and inheritance, and querying the resolved DOM.

## What This Demo Shows

### 1. HTML Parsing → DOM Tree

Given the following HTML:
```html
<div id="app">
  <header id="main-header" class="header dark">
    <h1>DOM Tree Demo</h1>
  </header>
  <main class="content">
    <section id="intro" class="section">
      <p class="text">Hello <span class="highlight">World</span>!</p>
    </section>
    <section id="details">
      <ul class="list">
        <li class="item">Item A</li>
        <li class="item">Item B</li>
        <li class="item active">Item C</li>
      </ul>
    </section>
  </main>
  <footer><p>Footer text</p></footer>
</div>
```

The HTML parser tokenizes this into:
- **Open tags**: `<div>`, `<header>`, `<h1>`, `<main>`, `<section>`, `<p>`, `<span>`, etc.
- **Text nodes**: "Hello ", "World", "Item A", etc.
- **Close tags**: `</div>`, `</header>`, etc.

The resulting DOM tree:
```
[DOCUMENT]
  <div id="app">
    <header id="main-header" class="header dark">
      <h1>
        "DOM Tree Demo"
    <main class="content">
      <section id="intro" class="section">
        <p class="text">
          "Hello "
          <span class="highlight">
            "World"
          "!"
      <section id="details">
        <ul class="list">
          <li class="item">
            "Item A"
          <li class="item">
            "Item B"
          <li class="item active">
            "Item C"
    <footer>
      <p>
        "Footer text"
```

### 2. CSS Parsing → CSSOM

The CSS is parsed into a collection of rules:
```css
body { margin: 0; padding: 0; }
#app { width: 100%; max-width: 960px; }
.header { background: #222; color: white; padding: 20px; }
.dark { font-weight: bold; }
.content { padding: 15px; }
.section { margin-bottom: 20px; }
.text { font-size: 16px; line-height: 1.6; }
.highlight { color: red; background: yellow; }
.list { display: flex; flex-direction: column; }
.item { padding: 8px; border-bottom: 1px solid #eee; }
.active { background: #e0f0ff; font-weight: bold; }
h1 { font-size: 28px; margin: 0; }
p { margin: 0 0 10px 0; }
```

Each rule gets a **specificity** score (a, b, c):
| Rule | Specificity | Explanation |
|---|---|---|
| `body` | (0, 0, 1) | 1 tag |
| `#app` | (1, 0, 0) | 1 ID |
| `.header` | (0, 1, 0) | 1 class |
| `.dark` | (0, 1, 0) | 1 class |
| `h1` | (0, 0, 1) | 1 tag |
| `.highlight` | (0, 1, 0) | 1 class |
| `.active` | (0, 1, 0) | 1 class |

### 3. Style Resolution → Computed Styles

The style resolver processes each element node through the cascade:

1. **Apply user-agent defaults** — based on tag name (e.g., `<div>` gets `display: block`, `<span>` gets `display: inline`, `<h1>` gets `font-size: 32px bold`)

2. **Match CSS rules** — for each element, iterate all rules and check if the selector matches:
   - `<div id="app">` matches `#app` (ID selector) and any tag selectors
   - `<header class="header dark">` matches `.header` and `.dark` (class selectors)
   - `<h1>` matches `h1` (tag selector)
   - `<span class="highlight">` matches `.highlight` (class selector)
   - `<li class="item active">` matches both `.item` and `.active` (class selectors)

3. **Resolve conflicts** — when multiple rules target the same property:
   - Higher specificity wins
   - Later declaration wins at equal specificity
   - `!important` overrides everything except another `!important`

4. **Inherit properties** — properties like `color`, `font-size`, `font-family` inherit from parent to child if not explicitly set

Example final computed styles:

**`<div id="app">`**:
```
display: block        (user-agent default)
width: 100%           (from #app rule)
max-width: 960px      (from #app rule)
margin: 8px           (inherited from body default)
```

**`<header class="header dark">`**:
```
display: block        (user-agent default)
background: #222      (from .header rule)
color: white          (from .header rule)
padding: 20px         (from .header rule)
font-weight: bold     (from .dark rule)
```

**`<span class="highlight">World</span>`**:
```
display: inline           (user-agent default for span)
color: red                (from .highlight rule)
background: yellow        (from .highlight rule)
font-size: 16px           (inherited from parent .text)
line-height: 1.6          (inherited from parent .text)
```

**`<li class="item active">Item C</li>`**:
```
padding: 8px              (from .item rule)
border-bottom: 1px solid #eee  (from .item rule)
background: #e0f0ff       (from .active rule, overrides .item)
font-weight: bold         (from .active rule)
```

### 4. DOM Queries

The demo demonstrates several query mechanisms:

- **`dom_get_element_by_id(document, "intro")`** — returns the `<section>` with id "intro". O(1) lookup through depth-first search.

- **`dom_get_elements_by_class_name(document, "item")`** — returns all 3 `<li>` elements with class "item" (including the one that also has "active").

- **`dom_query_selector(document, ".active")`** — returns the first element matching ".active" class selector.

### 5. DOM Manipulation

The demo programmatically creates and inserts a new element:
```c
DomNode* new_section = dom_node_create(DOM_NODE_ELEMENT, "section");
dom_node_set_attribute(new_section, "id", "new-section");
DomNode* new_p = dom_node_create(DOM_NODE_ELEMENT, "p");
dom_node_set_text(new_p, "Dynamically added paragraph!");
dom_node_append_child(new_section, new_p);
dom_node_append_child(app_element, new_section);
```

### 6. Node Cloning

Deep cloning duplicates an entire subtree:
```c
DomNode* clone = dom_node_clone(original, true);  // true = deep copy
```

## Running the Demo

```sh
make
bin/dom_example.exe
```

## Key Takeaways

- The DOM tree is a general tree with parent/child/sibling pointers — not limited to HTML elements.
- CSS specificity uses a 3-component system (a=IDs, b=classes/attributes/pseudo, c=tags). This mirrors the W3C CSS specification's specificity calculation.
- The cascade resolves conflicts deterministically: source priority → specificity → source order.
- Inheritance only applies to explicitly marked inheritable properties (about 15 properties in this implementation).
- User-agent defaults provide the base layer that all other styles build upon.
- The style resolver attaches computed style to each DOM node as a linked list — O(n) lookup but simple and works for demo purposes.
