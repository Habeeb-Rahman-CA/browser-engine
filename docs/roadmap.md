# Browser Engine Development Roadmap

This roadmap documents the implementation milestones of the educational C++20 browser engine from scratch.

---

## Milestone Status Summary

- [x] **Milestone 1**: Core Framework, Windowing, Event Loop & Diagnostics
- [x] **Milestone 2**: HTML Tokenizer (HTML5 state machine, character entities, doctypes)
- [x] **Milestone 3**: DOM Construction & Tree Debugging (`Document`, `Element`, `TextNode`, hierarchy, parser recovery)
- [x] **Milestone 4**: CSS Tokenizer, Parser & Computed Styles (selectors, combinators, specificity, inheritance, box properties)
- [x] **Milestone 5**: Block Layout & Box Model (dimensions, normal flow, auto-margins, anonymous blocks)
- [x] **Milestone 6**: Rendering, Display Lists & Text Rasterization (`DisplayList`, `DisplayListBuilder`, SDL text rasterizer)
- [x] **Milestone 7**: Scrolling, Hit Testing & Navigation (viewport clipping, mouse hit-testing, link navigation, scrollbars)
- [x] **Milestone 8**: Networking & Resource Loading (`Url`, `ResourceLoader`, libcurl HTTP/HTTPS, local file URLs, external stylesheet fetching)
- [x] **Milestone 9**: JavaScript Lexer, Parser & Expression Evaluator (lexer, AST, Pratt parser, values, operators)
- [x] **Milestone 10**: JavaScript Statements, Functions & Scopes (control flow, environments, closures, step & recursion limits)
- [x] **Milestone 11**: DOM Bindings & Interactive Scripts (`document`, `window`, `console.log`, `getElementById`, `addEventListener`, dynamic reflow)
- [x] **Milestone 12**: Dynamic Updates & Interactive Reflow (event dispatching, mutation detection, automatic re-style and layout)
- [x] **Milestone 13**: Performance Benchmarks & Developer Diagnostics (`browser_benchmarks`, subsystem profiler, CLI diagnostics)
- [x] **Milestone 14**: Verification & Documentation (51 unit tests, architecture documentation, CSS/HTML/JS support specifications)

---

## Subsystem Details

### Milestone 1: Core Framework, Windowing, Event Loop & Diagnostics
- Modern CMake build setup with `-Wall -Wextra -Wpedantic` clean compilation.
- Native windowing and graphics context abstraction using SDL3.
- Automatic display detection with headless fallback (dummy video driver).
- `FrameScheduler` regulating target 60 FPS timing and delta-time measurement.
- Decoupled `EventDispatcher` for window, mouse, keyboard, and scroll events.
- `BrowserDiagnostics` collecting subsystem timings, min/max metrics, and invocation counts.
- Browser chrome visualization with navigation buttons and address bar.

### Milestone 2: HTML Tokenizer
- State-machine-based tokenizer following HTML5 specification principles.
- Tokenization of doctypes, start tags, end tags, attributes, text runs, comments, and EOF.
- Named and numeric character reference resolution (`&amp;`, `&lt;`, `&#60;`, `&#x3C;`).
- Recovery from unquoted attributes and malformed markup.

### Milestone 3: DOM Construction & Tree Debugging
- Full DOM tree model: `Document`, `Element`, `TextNode`, `CommentNode`.
- Tree mutation primitives: `appendChild`, `removeChild`, `insertBefore`, `setTextContent`.
- Query APIs: `getElementById`, `getElementsByTagName`.
- HTML tree builder with open-elements stack and automatic recovery for implicit `<html>`, `<head>`, `<body>`, and auto-closing `<p>`.

### Milestone 4: CSS Tokenizer, Parser & Computed Styles
- Tokenizer for dimensions, hex colors, identifiers, strings, and operators.
- Parser for rules, complex selector lists, declarations, and shorthand properties (`margin`, `padding`, `border`).
- Selector matching for type, class, ID, child (`>`), and descendant (` `) combinators.
- Specificity calculation, cascade ordering, origin weighting, and property inheritance.

### Milestone 5: Block Layout & Box Model
- `LayoutTreeBuilder` mapping DOM elements to `LayoutBox` trees, skipping `display: none` and inserting anonymous blocks.
- Block formatting model: content dimensions, padding, borders, margins, auto margin centering (`margin: 0 auto`), and vertical positioning.

### Milestone 6 & 7: Rendering, Scrolling, Hit Testing & Navigation
- Retained `DisplayList` and `DisplayListBuilder` decoupling geometry from rendering.
- Debug text rasterization with measured glyph dimensions and box clipping.
- Vertical scrolling with mouse wheel and interactive scrollbar rendering.
- Point hit testing on layout boxes to identify elements and hyperlinks under the cursor.
- Navigation history stack with back, forward, and reload capabilities.

### Milestone 8: Networking & Resource Loading
- `Url` parser supporting `file://`, `http://`, `https://`, and `about:` schemes.
- Relative URL resolution against base documents.
- `ResourceLoader` supporting synchronous fetching via libcurl with TLS verification and local file streaming.
- Automatic fetching and cascade application of external stylesheets (`<link rel="stylesheet">`).

### Milestone 9 & 10: Educational JavaScript Runtime
- Lexer: Identifiers, numbers, strings with escape sequences, comments, keywords, and operators.
- AST & Parser: Binary precedence expressions, assignments, member expressions, unary operators, function declarations, `return`, `if/else`, `while`, `for`.
- Runtime: Dynamic `Value` type (number, string, boolean, null, undefined, object, array, function).
- Lexical Scopes & Closures: Nested `Environment` scopes and closure capture.
- Resource Limits: Instruction step limit and recursion depth bounds to prevent infinite loops and stack exhaustion.

### Milestone 11 & 12: DOM Bindings & Dynamic Reflow
- Host environment globals: `console.log`, `window`, `document`.
- Element manipulation: `getElementById`, `getElementsByTagName`, `createElement`, `createTextNode`, `appendChild`, `removeChild`, `setAttribute`, `getAttribute`, `setTextContent`, `setInnerHTML`.
- Event handling: `element.addEventListener('click', fn)` and event bubbling dispatching.
- Dynamic reflow: Script DOM mutations automatically trigger style resolution, tree layout, and repaint.

### Milestone 13 & 14: Benchmarking & Developer Tools
- Full test suite: 51 test cases with 291 assertions passing without errors.
- End-to-end benchmark suite (`browser_benchmarks`):
  - HTML Tokenize & Parse: ~4,100 docs/sec
  - CSS Tokenize & Parse: ~6,600 sheets/sec
  - Style Resolution: ~6,000 ops/sec
  - Layout Computation: ~34,500 layouts/sec
  - JavaScript Execution: ~2,500 execs/sec
  - Full Pipeline (HTML -> Display List): ~1,140 complete rendering passes/sec (< 0.88 ms/frame).
- Developer CLI: `--dump-tokens`, `--dump-dom`, `--dump-style`, `--dump-layout`, `--dump-display-list`, `--eval-js`, `--headless`, `--frames`.
