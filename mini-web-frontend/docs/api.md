# mini-web-frontend API Reference

## html_parser.h

### Types

```c
typedef enum { HTML_TOKEN_NONE, HTML_TOKEN_OPEN_TAG, HTML_TOKEN_CLOSE_TAG,
               HTML_TOKEN_SELF_CLOSING_TAG, HTML_TOKEN_TEXT,
               HTML_TOKEN_COMMENT, HTML_TOKEN_DOCTYPE } HtmlTokenType;

typedef struct {
    char* name;
    char* value;
} HtmlAttribute;

typedef struct {
    HtmlTokenType type;
    char* tag_name;
    HtmlAttribute* attributes;
    size_t attr_count, attr_capacity;
    char* text_content;
    bool is_void_element;
} HtmlToken;

typedef struct {
    const char* source;
    size_t source_len, pos;
    HtmlToken current_token;
    bool has_error;
    char error_message[256];
} HtmlParser;
```

### Functions

| Function | Description |
|---|---|
| `html_parser_init(HtmlParser*, const char*)` | Initialize parser with HTML string |
| `html_parser_free(HtmlParser*)` | Free current token memory |
| `html_parser_next_token(HtmlParser*)` | Advance to next token, return true if token available |
| `html_parse_document(const char*)` | Parse HTML string into full DOM document |
| `html_parse_fragment(const char*)` | Parse HTML string into flat fragment node |
| `html_is_void_element(const char*)` | Check if tag is a void/self-closing element |

---

## css_parser.h

### Types

```c
typedef enum { CSS_SELECTOR_UNIVERSAL, CSS_SELECTOR_TAG, CSS_SELECTOR_CLASS,
               CSS_SELECTOR_ID, CSS_SELECTOR_ATTRIBUTE, CSS_SELECTOR_PSEUDO } CssSelectorType;

typedef enum { CSS_COMBINATOR_NONE, CSS_COMBINATOR_DESCENDANT, CSS_COMBINATOR_CHILD,
               CSS_COMBINATOR_ADJACENT_SIBLING, CSS_COMBINATOR_GENERAL_SIBLING } CssCombinatorType;

typedef struct CssSelectorPart { ... } CssSelectorPart;
typedef struct CssSelector { ... } CssSelector;
typedef struct CssDeclaration { ... } CssDeclaration;
typedef struct CssRule { ... } CssRule;
typedef struct CssStylesheet { ... } CssStylesheet;
typedef struct { ... } CssParser;
```

### Functions

| Function | Description |
|---|---|
| `css_parse_stylesheet(const char*)` | Parse CSS string into stylesheet object |
| `css_stylesheet_free(CssStylesheet*)` | Free stylesheet and all rules/declarations |
| `css_selector_create()` | Create empty selector |
| `css_selector_add_part(CssSelector*, CssSelectorType, const char*)` | Add selector part (tag, class, id, etc.) |
| `css_selector_calculate_specificity(CssSelector*)` | Compute (a,b,c) specificity |
| `css_compare_specificity(a, b)` | Returns 0=equal, 1=a higher, 2=b higher |
| `css_rule_add_declaration(CssRule*, property, value, important)` | Add declaration to rule |

---

## dom_tree.h

### Types

```c
typedef enum { DOM_NODE_ELEMENT, DOM_NODE_TEXT,
               DOM_NODE_COMMENT, DOM_NODE_DOCUMENT } DomNodeType;

typedef struct DomAttribute { char* name; char* value; } DomAttribute;
typedef struct ComputedStyle { char* property; char* value; bool important; ... } ComputedStyle;
typedef struct DomNode { ... } DomNode;
typedef void (*DomNodeVisitor)(DomNode* node, void* user_data);
```

### Functions — Node Lifecycle

| Function | Description |
|---|---|
| `dom_node_create(DomNodeType, const char* tag_name)` | Create node |
| `dom_node_free(DomNode*)` | Free single node (not children) |
| `dom_node_free_recursive(DomNode*)` | Free node and all descendants |
| `dom_node_clone(const DomNode*, bool deep)` | Clone node (deep=true copies subtree) |

### Functions — Tree Manipulation

| Function | Description |
|---|---|
| `dom_node_append_child(parent, child)` | Append child to parent's end |
| `dom_node_insert_before(parent, new_node, ref_node)` | Insert new_node before ref_node |
| `dom_node_remove_child(parent, child)` | Remove child from parent |
| `dom_node_set_text(DomNode*, const char*)` | Set text content on text/comment nodes |

### Functions — Attributes

| Function | Description |
|---|---|
| `dom_node_set_attribute(node, name, value)` | Set or update attribute |
| `dom_node_get_attribute(node, name)` | Get attribute value or NULL |
| `dom_node_has_attribute(node, name)` | Check attribute existence |
| `dom_node_remove_attribute(node, name)` | Remove attribute |

### Functions — Query

| Function | Description |
|---|---|
| `dom_get_element_by_id(root, id)` | Find first element with matching id |
| `dom_get_elements_by_tag_name(root, tag, *count)` | Find all elements by tag name |
| `dom_get_elements_by_class_name(root, class, *count)` | Find all elements by class name |
| `dom_query_selector(root, selector)` | Find first match (supports #id, .class, tag) |
| `dom_query_selector_all(root, selector, *count)` | Find all matches |

### Functions — Traversal

| Function | Description |
|---|---|
| `dom_traverse_preorder(root, visitor, user_data)` | Preorder depth-first traversal |
| `dom_traverse_postorder(root, visitor, user_data)` | Postorder depth-first traversal |
| `dom_print_tree(root, indent)` | Pretty-print DOM tree to stdout |

### Functions — Computed Styles

| Function | Description |
|---|---|
| `dom_node_set_computed_style(node, property, value, important)` | Set computed style |
| `dom_node_get_computed_style(node, property)` | Get computed style value |
| `dom_node_clear_computed_styles(node)` | Remove all computed styles |

---

## style_resolver.h

### Types

```c
typedef enum { STYLE_SOURCE_USER_AGENT, STYLE_SOURCE_USER, STYLE_SOURCE_AUTHOR,
               STYLE_SOURCE_AUTHOR_IMPORTANT, STYLE_SOURCE_USER_IMPORTANT,
               STYLE_SOURCE_INLINE, STYLE_SOURCE_INLINE_IMPORTANT } StyleSource;

typedef struct StyleProperty { ... } StyleProperty;
typedef struct StyleMap { ... } StyleMap;
typedef struct { ... } StyleResolver;
```

### Functions

| Function | Description |
|---|---|
| `style_resolve_document(DomNode*, CssStylesheet*)` | Resolve all styles for entire document |
| `style_resolve_node(DomNode*, CssStylesheet*, StyleMap* parent)` | Resolve single subtree |
| `style_match_selector(node, selector)` | Check if node matches CSS selector |
| `style_inherit_properties(parent_map, child_map)` | Copy inheritable properties |
| `style_compute_defaults(map, tag_name)` | Apply user-agent default styles |
| `style_resolver_set_inline_style(node, property, value, important)` | Set inline style directly |
| `style_resolver_print_computed(node)` | Print computed styles to stdout |
| `style_map_set(map, property, value, source, specificity)` | Set property with cascade info |
| `style_map_get(map, property)` | Get property value |
| `style_find_conflict(map, property)` | Find conflicting property entry |

---

## layout_engine.h

### Types

```c
typedef enum { DISPLAY_BLOCK, DISPLAY_INLINE, DISPLAY_INLINE_BLOCK,
               DISPLAY_FLEX, DISPLAY_INLINE_FLEX, DISPLAY_NONE } DisplayType;

typedef enum { POSITION_STATIC, POSITION_RELATIVE,
               POSITION_ABSOLUTE, POSITION_FIXED } PositionType;

typedef enum { FLEX_DIRECTION_ROW, FLEX_DIRECTION_ROW_REVERSE,
               FLEX_DIRECTION_COLUMN, FLEX_DIRECTION_COLUMN_REVERSE } FlexDirection;

typedef enum { JUSTIFY_FLEX_START, JUSTIFY_FLEX_END, JUSTIFY_CENTER,
               JUSTIFY_SPACE_BETWEEN, JUSTIFY_SPACE_AROUND,
               JUSTIFY_SPACE_EVENLY } JustifyContent;

typedef enum { ALIGN_FLEX_START, ALIGN_FLEX_END, ALIGN_CENTER,
               ALIGN_STRETCH, ALIGN_BASELINE } AlignItems;

typedef struct { float margin[4], border[4], padding[4], content_w, content_h; } BoxModel;
typedef struct LayoutBox { ... } LayoutBox;
typedef struct { float viewport_w, viewport_h, root_font_size; LayoutBox* root_box; } LayoutContext;
```

### Functions

| Function | Description |
|---|---|
| `layout_box_create()` | Create empty layout box |
| `layout_box_free(LayoutBox*)` | Free single box |
| `layout_box_free_recursive(LayoutBox*)` | Free box and all children |
| `layout_box_append_child(parent, child)` | Add child layout box |
| `layout_context_init(ctx, vw, vh)` | Initialize layout context with viewport dimensions |
| `layout_context_free(ctx)` | Free layout context and root box |
| `layout_compute(DomNode*, LayoutContext*)` | Build and compute full layout tree |
| `layout_compute_box(box, ctx, available_width)` | Compute single box's layout |
| `layout_compute_flex(flex_box, ctx)` | Compute flex container layout |
| `layout_compute_position(box, ctx)` | Compute absolute/fixed position |
| `box_model_default()` | Return zero-initialized BoxModel |
| `box_model_compute_size(box, avail_w, avail_h)` | Compute box content size from available space |
| `box_model_total_width(box)` | Total width (margin+border+padding+content) |
| `box_model_total_height(box)` | Total height |
| `box_model_content_x(box)` | X offset to content area |
| `box_model_content_y(box)` | Y offset to content area |
| `layout_print_tree(root, indent)` | Print layout tree with positions and sizes |

---

## Common Patterns

### Parse and Render Full Page

```c
DomNode* doc = html_parse_document(html_string);
CssStylesheet* css = css_parse_stylesheet(css_string);
style_resolve_document(doc, css);

LayoutContext ctx;
layout_context_init(&ctx, 1024, 768);
LayoutBox* layout = layout_compute(doc, &ctx);

layout_print_tree(layout, 0);

layout_context_free(&ctx);
css_stylesheet_free(css);
dom_node_free_recursive(doc);
```

### Query and Manipulate

```c
DomNode* elem = dom_get_element_by_id(doc, "my-id");
if (elem) {
    dom_node_set_attribute(elem, "class", "active");
    const char* color = dom_node_get_computed_style(elem, "color");
    printf("Color: %s\n", color);
}
```

### Walk the Tree

```c
void my_visitor(DomNode* node, void* data) {
    if (node->type == DOM_NODE_ELEMENT) {
        printf("Found: <%s>\n", node->tag_name);
    }
}
dom_traverse_preorder(doc, my_visitor, NULL);
```
