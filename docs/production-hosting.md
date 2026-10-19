# Production Hosting & Web Deployment Guide

This guide describes how to host the **Educational Browser Engine** so that anyone can access, interact with, test, and inspect the engine remotely over the web.

---

## 1. Architecture Overview

When hosted, the engine runs as `browser_server`: a lightweight, high-performance, asynchronous C++20 HTTP server with zero heavy web-framework dependencies.

```
                              HTTP / HTTPS
Client Web Browser ───────────────────────────────────► Nginx / Caddy (SSL Reverse Proxy)
(Desktop / Mobile)                                              │
                                                                ▼ (Port 8080)
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                                 browser_server                                         │
│                                                                                        │
│  ┌─────────────────────────┐  ┌─────────────────────────────────────────────────────┐  │
│  │   Embedded Workbench    │  │                   RESTful API                       │  │
│  │   Interactive SPA       │  │  /health  /api/navigate  /api/click  /api/eval      │  │
│  └─────────────────────────┘  └─────────────────────────────────────────────────────┘  │
│                                           │                                            │
│                                           ▼                                            │
│  ┌──────────────────────────────────────────────────────────────────────────────────┐  │
│  │                     Headless C++20 Browser Engine Instance                       │  │
│  │  HTML Parser ──► DOM ──► CSS Cascade ──► Layout Box ──► Display List ──► SVG/BMP │  │
│  │  JavaScript Interpreter (Lexer + AST + Runtime + DOM Bindings)                   │  │
│  └──────────────────────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

### Key Capabilities When Hosted
* **Live Vector Viewport**: Renders the engine's exact display list into razor-sharp, Retina-ready SVG.
* **Interactive Clicking**: Clicking anywhere on the rendered canvas forwards coordinates `(x, y)` to the engine's hit tester, triggering DOM events, JavaScript mutation handlers, reflow, and hyperlink navigation.
* **Deep Inspector Tabs**:
  * **DOM Tree**: Live tree view with tag badges, attributes, and text nodes.
  * **Computed Styles**: Exact CSS computed properties (colors, dimensions, margins, paddings, border radii).
  * **Layout Geometry**: Full box model metrics (content box, padding box, border box, margin box).
  * **Display List**: The exact paint command stream (`FillRect`, `StrokeRect`, `Text`, `PushClip`, `PopClip`).
  * **Interactive JavaScript REPL**: Execute JavaScript expressions in real time against the live DOM context.
  * **Engine Diagnostics**: Subsystem duration breakdowns with microsecond accuracy.

---

## 2. Deployment Option A: Docker Compose (Recommended)

The simplest, zero-dependency way to host on any Linux server, Mac, or Windows cloud VM:

```bash
# Clone the repository
git clone https://github.com/your-username/browser-engine.git
cd browser-engine

# Build and start container in the background
docker compose up -d --build

# View container logs
docker compose logs -f
```

The engine is now live at `http://<your-server-ip>:8080`.

To stop the service:
```bash
docker compose down
```

---

## 3. Deployment Option B: Native Linux Systemd Service

For direct hosting on Ubuntu, Debian, or Fedora without Docker:

### 1. Build and Install Optimized Binaries
```bash
cmake -B build -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/browser-engine

cmake --build build -j$(nproc)
sudo cmake --install build
```

### 2. Configure Systemd Daemon
```bash
# Create application user
sudo useradd -r -s /bin/false browseruser

# Install systemd unit
sudo cp browser-engine.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now browser-engine

# Check service status
sudo systemctl status browser-engine
```

---

## 4. Reverse Proxy Setup (HTTPS with Nginx & Caddy)

### Nginx Configuration
Place this in `/etc/nginx/sites-available/browser-engine`:

```nginx
server {
    listen 80;
    server_name browser.yourdomain.com;
    return 301 https://$host$request_uri;
}

server {
    listen 443 ssl http2;
    server_name browser.yourdomain.com;

    ssl_certificate /etc/letsencrypt/live/browser.yourdomain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/browser.yourdomain.com/privkey.pem;

    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        # WebSocket & Streaming support
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
```

### Caddy Configuration (Automatic SSL)
```caddy
browser.yourdomain.com {
    reverse_proxy 127.0.0.1:8080
}
```

---

## 5. REST API Reference

The server exposes programmatic REST endpoints for automation, headless testing, and integrations:

### `GET /health`
Production health-check endpoint for load balancers.
* **Response**: `200 OK`
```json
{
  "status": "healthy",
  "engine": "educational-browser-engine",
  "version": "0.1.0"
}
```

### `POST /api/navigate`
Loads a URL or raw HTML string into the engine and returns the rendered frame and complete state.
* **Payload**:
```json
{
  "url": "examples/css/index.html"
}
```
*or for custom raw markup:*
```json
{
  "html": "<!DOCTYPE html><html><body><h1>Hello World</h1></body></html>",
  "baseUrl": "about:blank"
}
```
* **Response**: `200 OK` with `{ url, svg, dom, styles, layout, displayList, diagnostics }`.

### `POST /api/click`
Simulates a mouse click at `(x, y)` in the engine viewport.
* **Payload**:
```json
{
  "x": 240,
  "y": 320
}
```
* **Response**: `200 OK` with updated state after event dispatching and reflow.

### `POST /api/eval`
Evaluates a JavaScript snippet in the page context.
* **Payload**:
```json
{
  "code": "document.title"
}
```
* **Response**:
```json
{
  "result": "Educational Browser Engine Demo",
  "state": { ... }
}
```

### `GET /api/diagnostics`
Returns real-time profiling metrics for all engine subsystems (HTML tokenizer, parser, style resolution, layout, rendering).

---

## 6. Security & Sandboxing Considerations

When hosting for public access:
1. **Loop & Resource Limits**: The JavaScript interpreter enforces strict step limits (default: 500 steps) to prevent DoS via infinite loops (`while (true) {}`).
2. **Network Timeouts**: Resource loading over HTTP/HTTPS enforces connection and transfer timeouts via libcurl.
3. **Non-Root Execution**: Both Docker and Systemd execute under unprivileged user accounts with restricted filesystem access (`ProtectSystem=strict`).
4. **Input Size Validation**: Payloads exceeding 64KB are rejected to prevent buffer exhaustion.

