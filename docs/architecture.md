# Browser Engine Architecture

This document specifies the architectural patterns, ownership rules, module boundaries, and subsystem contracts governing the educational browser engine.

---

## 1. Architectural Philosophy

1. **Separation of Concerns**: Parsing, style resolution, geometry calculation, display list generation, and painting are distinct, strictly isolated stages. A layout box never paints directly to a pixel buffer; a DOM node never calculates screen coordinates.
2. **Explicit Data Ownership**: Ownership is managed through standard C++ RAII primitives (`std::unique_ptr`, `std::shared_ptr`, and value semantics). Circular dependencies are forbidden.
3. **Observability by Design**: Every pipeline phase exports structured timing telemetry, invocation counters, and formatted debug representations via `BrowserDiagnostics`.
4. **Resilience & Graceful Degradation**: If hardware display devices or native window servers are unavailable (such as in headless CI environments), the engine falls back to dummy/offscreen rendering rather than failing.

---

## 2. Subsystem Directory Structure

```
browser-engine/
  ├── include/browser/
  │   ├── core/         # Primitives (Point, Rect, Color), BrowserDiagnostics, ScopedTimer
  │   ├── platform/     # OS windowing, event types, input abstractions
  │   ├── events/       # EventDispatcher and decoupled subscription mechanics
  │   ├── shell/        # FrameScheduler, BrowserEngine coordinator
  │   ├── networking/   # ResourceLoader, URL parsing, cache, curl backend (Milestone 8)
  │   ├── html/         # HtmlTokenizer, HtmlParser, CharacterReferences (Milestones 2 & 3)
  │   ├── dom/          # Node, Element, Document, TextNode, MutationObservers (Milestone 3)
  │   ├── css/          # CssTokenizer, CssParser, RuleSet (Milestone 4)
  │   ├── style/        # SelectorMatcher, StyleResolver, ComputedStyle (Milestone 4)
  │   ├── layout/       # LayoutEngine, LayoutBox, InlineFormatting, BlockFlow (Milestones 5 & 6)
  │   ├── graphics/     # DisplayList, DisplayListBuilder, PaintContext, Rasterizer (Milestones 6 & 8)
  │   └── javascript/   # Lexer, Parser, AST, Interpreter, DomBindings (Milestones 9, 10 & 11)
  ├── src/              # Subsystem implementation files
  ├── tests/            # Unit, integration, and layout verification tests
  └── docs/             # Technical specifications and standard support docs
```

---

## 3. Core Component Interfaces

### `BrowserEngine` (`include/browser/shell/engine.hpp`)
The root application coordinator. Manages:
- Subsystem lifecycles (Window, EventDispatcher, FrameScheduler).
- Engine configuration (window size, initial URL, headless mode, frame limits).
- The primary game/render loop ticking input polling, engine updates, rendering, and frame rate throttling.

### `BrowserWindow` (`include/browser/platform/window.hpp`)
Encapsulates native window management and the graphics context:
- Uses SDL3 for cross-platform windowing.
- Automatically handles Wayland/X11 display detection with headless fallback to the SDL dummy driver.
- Translates platform-specific events to strongly typed `platform::Event` instances.
- Exposes primitive 2D drawing capabilities for the browser chrome and rasterization backends.

### `EventDispatcher` (`include/browser/events/event_dispatcher.hpp`)
Decoupled observer pattern:
- Supports type-specific subscriptions (`subscribe(EventType, callback)`) and global subscriptions (`subscribeAll(callback)`).
- Safe against re-entrant unsubscription during callback iteration.

### `FrameScheduler` (`include/browser/shell/frame_scheduler.hpp`)
Ensures deterministic frame pacing:
- Regulates target frame rates (default 60.0 FPS) using high-resolution steady clocks.
- Computes delta time (`deltaTime()`) with upper and lower clamping to prevent physics/animation spirals of death.
- Smooths and reports rolling frames-per-second (FPS) metrics.

### `BrowserDiagnostics` (`include/browser/core/diagnostics.hpp`)
Provides comprehensive telemetry:
- Thread-safe collection of invocation counts, total runtime, min duration, and max duration across all subsystems.
- RAII `ScopedTimer` for micro-benchmarking individual operations.
- ANSI-colored structured console logging with timestamping.
- Tabular CLI performance summaries (`dumpSummary()`).

