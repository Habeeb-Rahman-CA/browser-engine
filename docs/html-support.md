# HTML Support Specification

This document defines the supported HTML grammar, tokens, element models, and tree construction rules implemented across Milestones 2 and 3.

---

## 1. Supported Token Types

The `HtmlTokenizer` will recognize and emit:
- `DOCTYPE`: Root document type declaration.
- `StartTag`: Element name, self-closing flag, and attribute key-value pairs.
- `EndTag`: Element closing tag name.
- `Character`: Raw text content characters.
- `Comment`: Comment strings (`<!-- ... -->`).
- `EndOfFile`: Stream termination indicator.

---

## 2. Character Reference Decoding

Supported character entity references:
- Named entities: `&amp;`, `&lt;`, `&gt;`, `&quot;`, `&apos;`, `&nbsp;`.
- Decimal numeric character references: `&#60;` -> `<`.
- Hexadecimal numeric character references: `&#x3C;` -> `<`.

---

## 3. Supported Elements & Tags

| Category | Elements |
| :--- | :--- |
| **Document Structure** | `html`, `head`, `body`, `title`, `meta`, `link`, `style`, `script` |
| **Content Sections** | `h1`, `h2`, `h3`, `h4`, `h5`, `h6`, `p`, `div`, `section`, `article`, `header`, `footer` |
| **Inline Text** | `span`, `a`, `strong`, `b`, `em`, `i`, `code` |
| **Lists** | `ul`, `ol`, `li` |
| **Replaced / Void** | `img`, `br`, `hr`, `input`, `meta`, `link` |

---

## 4. DOM Interface Subset

Nodes expose:
- `nodeType`: `Element`, `Text`, `Document`, `Comment`.
- `nodeName`: Tag name (uppercase) or `#text`, `#document`.
- Hierarchical navigation: `parentNode`, `firstChild`, `lastChild`, `nextSibling`, `previousSibling`, `childNodes`.
- Tree mutation: `appendChild(node)`, `removeChild(node)`, `insertBefore(newNode, refNode)`.
- Element querying: `getElementById(id)`, `getElementsByTagName(tag)`.

