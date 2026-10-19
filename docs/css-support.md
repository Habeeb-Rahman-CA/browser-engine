# CSS Support Specification

This document defines the supported CSS selectors, properties, values, and cascade computation rules implemented in Milestone 4.

---

## 1. Selectors

| Selector Type | Syntax | Example | Specificity |
| :--- | :--- | :--- | :--- |
| **Type Selector** | `element` | `div`, `p`, `h1` | `(0, 0, 1)` |
| **Class Selector** | `.class` | `.container`, `.active` | `(0, 1, 0)` |
| **ID Selector** | `#id` | `#main-nav`, `#header` | `(1, 0, 0)` |
| **Descendant Combinator** | `ancestor descendant` | `div p`, `nav a` | Sum of components |
| **Child Combinator** | `parent > child` | `ul > li` | Sum of components |
| **Selector List** | `sel1, sel2` | `h1, h2, h3` | Evaluated individually |
| **Pseudo-Classes** | `:hover`, `:focus` | `a:hover`, `button:focus` | `(0, 1, 0)` |

---

## 2. Supported CSS Properties

### Box Model & Dimensions
- `width`, `height` (pixels, percentages, `auto`)
- `margin`, `margin-top`, `margin-right`, `margin-bottom`, `margin-left`
- `padding`, `padding-top`, `padding-right`, `padding-bottom`, `padding-left`
- `border-width`, `border-style`, `border-color`, `border` shorthand
- `display`: `block`, `inline`, `inline-block`, `none`

### Colors & Backgrounds
- `color`: Hexadecimal (`#fff`, `#ffffff`, `#ffffff80`), named colors (`black`, `white`, `red`, `blue`, `green`, `gray`), `rgb()`, `rgba()`
- `background-color`: Same color formats

### Typography
- `font-size`: pixels (`12px`, `16px`), points (`12pt`), keywords
- `font-weight`: `normal`, `bold`, numeric weights (`400`, `700`)
- `text-align`: `left`, `right`, `center`
- `line-height`: unitless multipliers, pixels

