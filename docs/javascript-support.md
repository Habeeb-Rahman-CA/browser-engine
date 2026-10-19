# JavaScript Support Specification

This document defines the custom, educational JavaScript runtime implemented across Milestones 9, 10, and 11.

---

## 1. Syntax & Core Language Features

- **Data Types**: `Number` (double-precision float), `String`, `Boolean`, `Null`, `Undefined`, `Object`, `Array`, `Function`.
- **Variables & Scoping**: `let`, `const`, `var` with lexical block scopes and function scopes.
- **Operators**:
  - Arithmetic: `+`, `-`, `*`, `/`, `%`
  - Comparison: `==`, `!=`, `===`, `!==`, `<`, `<=`, `>`, `>=`
  - Logical: `&&`, `||`, `!`
  - Assignment: `=`, `+=`, `-=`, `*=`, `/=`
- **Control Flow**: `if / else`, `while`, `for`, `return`, `break`, `continue`.
- **Functions**: Function declarations, function calls, parameter passing, return statements, and lexical closures.
- **Objects & Arrays**: Object literals (`{ key: value }`), array literals (`[1, 2, 3]`), member access (`obj.prop`, `arr[0]`), property assignment.

---

## 2. Resource & Safety Bounds

To ensure educational safety and prevent hangs:
- **Max Instruction Limit**: Prevents infinite `while(true)` loops from stalling the browser main thread.
- **Call Stack Depth Limit**: Prevents stack overflow crashes on unbounded recursion.
- **Memory Allocation Cap**: Bounds heap object count to mitigate memory exhaustion.

---

## 3. DOM & Host Environment Bindings

Exposed JavaScript globals:
- `console.log(...)`
- `window`
- `document.title`
- `document.body`
- `document.getElementById(id)`
- `document.createElement(tagName)`
- `node.appendChild(child)`
- `node.removeChild(child)`
- `node.textContent`
- `element.setAttribute(name, value)`
- `element.getAttribute(name)`
- `element.addEventListener('click', handler)`

