# mini-ui-ux-hci — UI/UX与HCI (C 语言实现)

A comprehensive C99 library for UI/UX design primitives and Human-Computer Interaction patterns. Generates CSS custom properties, accessible HTML, and theme-aware component catalogs.

## Modules

| Module | Header | Description |
|--------|--------|-------------|
| **Design Tokens** | `design_tokens.h` | Color palette, spacing scale (4px base), typography scale, border radius, shadow levels. Token layers: global → alias → component. CSS custom property generation. Dark mode token overrides. Token inheritance. |
| **Component Library** | `component_library.h` | Catalog of 23 components (Button, Input, Card, Modal, Table, etc.). Props: variant, size, disabled. States: default, hover, active, focus, disabled, loading. Storybook-like demo catalog. |
| **Accessibility** | `accessibility.h` | ARIA roles (button, navigation, main, dialog, alert). ARIA properties (aria-label, aria-expanded, aria-selected). Keyboard navigation (Tab order, arrow keys). Focus management. Screen reader labels. Contrast ratio checker. WCAG 2.1 AA compliance checker. |
| **Interaction Model** | `interaction_model.h` | Touch/pointer events (touchstart→touchmove→touchend). Gesture recognition: tap, double-tap, long-press, swipe, pinch. Click delay model (300ms elimination). Hover/active states. Cursor states. Drag and drop model. |
| **Theme System** | `theme_system.h` | Light/dark mode switching. Theme provider with CSS variables. color-scheme detection. System preference (prefers-color-scheme). Custom themes. Theme context propagation. Component-level theme override. Transition animation on theme change. |

## Quick Start

```bash
make
make examples
make run-demos
```

## Structure

```
src/         — 5 headers + 5 sources (C99 library)
examples/    — 3 standalone examples
demos/       — 2 full demo applications
docs/        — API reference & developer guide
```

## Design Token Layers

```
Global (raw values)     →  Alias (semantic)     →  Component (scoped)
--color-blue-500:#3b82f6  --color-primary:var()    --btn-bg-primary:var()
```

## WCAG 2.1 Compliance

- Contrast ratio checker (luminance calculation per WCAG)
- AA standard: 4.5:1 normal text, 3:1 large text
- AAA standard: 7:1 normal text, 4.5:1 large text
- Automated audits for 50+ WCAG criteria

## Theme System

```css
:root { --color-bg-primary: #ffffff; /* ... */ }

@media (prefers-color-scheme: dark) {
  :root { --color-bg-primary: #0f172a; /* ... */ }
}

/* Smooth transition on theme change */
* { transition: background-color 300ms ease-in-out, color 300ms ease-in-out; }
```

## License

MIT
