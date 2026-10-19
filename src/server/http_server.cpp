#include "browser/server/http_server.hpp"
#include "browser/core/serializer.hpp"
#include "browser/core/diagnostics.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <cstring>
#include <algorithm>

namespace browser::server {

namespace {

std::string extractJsonString(std::string_view json, std::string_view key) {
    std::string search = "\"" + std::string(key) + "\"";
    auto pos = json.find(search);
    if (pos == std::string_view::npos) return "";
    pos = json.find(':', pos + search.size());
    if (pos == std::string_view::npos) return "";
    pos = json.find('"', pos);
    if (pos == std::string_view::npos) return "";
    size_t start = pos + 1;
    size_t end = start;
    bool escaped = false;
    while (end < json.size()) {
        if (escaped) {
            escaped = false;
        } else if (json[end] == '\\') {
            escaped = true;
        } else if (json[end] == '"') {
            break;
        }
        ++end;
    }
    
    // Unescape basic json characters
    std::string result;
    for (size_t i = start; i < end; ++i) {
        if (json[i] == '\\' && i + 1 < end) {
            char next = json[++i];
            if (next == 'n') result += '\n';
            else if (next == 'r') result += '\r';
            else if (next == 't') result += '\t';
            else if (next == '"') result += '"';
            else if (next == '\\') result += '\\';
            else result += next;
        } else {
            result += json[i];
        }
    }
    return result;
}

int extractJsonInt(std::string_view json, std::string_view key, int defaultValue = 0) {
    std::string search = "\"" + std::string(key) + "\"";
    auto pos = json.find(search);
    if (pos == std::string_view::npos) return defaultValue;
    pos = json.find(':', pos + search.size());
    if (pos == std::string_view::npos) return defaultValue;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':')) ++pos;
    try {
        return std::stoi(std::string(json.substr(pos)));
    } catch (...) {
        return defaultValue;
    }
}

} // namespace

HttpServer::HttpServer(ServerConfig config)
    : m_config(std::move(config)) {
    // Initialize headless BrowserEngine instance
    shell::EngineConfig engineConfig;
    engineConfig.window.headless = true;
    engineConfig.window.width = static_cast<int>(m_config.viewportWidth);
    engineConfig.window.height = static_cast<int>(m_config.viewportHeight);
    engineConfig.initialUrl = "about:blank";
    engineConfig.dumpDiagnosticsOnExit = false;

    m_engine = std::make_unique<shell::BrowserEngine>(engineConfig);
    if (!m_engine->initialize()) {
        LOG_ERROR("HttpServer", "Failed to initialize headless BrowserEngine instance");
    }
}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::start() {
    m_serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverFd < 0) {
        LOG_ERROR("HttpServer", "Failed to create socket");
        return false;
    }

    int opt = 1;
    setsockopt(m_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(m_config.port);

    if (bind(m_serverFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        LOG_ERROR("HttpServer", "Failed to bind to port " + std::to_string(m_config.port));
        close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    if (listen(m_serverFd, 32) < 0) {
        LOG_ERROR("HttpServer", "Failed to listen on socket");
        close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    m_running.store(true);
    LOG_INFO("HttpServer", "Educational Browser Engine Server running on http://" + m_config.host + ":" + std::to_string(m_config.port));

    m_serverThread = std::thread(&HttpServer::runServerLoop, this);
    return true;
}

void HttpServer::stop() {
    if (!m_running.exchange(false)) return;
    if (m_serverFd >= 0) {
        shutdown(m_serverFd, SHUT_RDWR);
        close(m_serverFd);
        m_serverFd = -1;
    }
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
    LOG_INFO("HttpServer", "Server stopped cleanly.");
}

void HttpServer::runServerLoop() {
    while (m_running.load()) {
        pollfd pfd{};
        pfd.fd = m_serverFd;
        pfd.events = POLLIN;

        int ret = poll(&pfd, 1, 250);
        if (ret <= 0) continue;

        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(m_serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSocket < 0) {
            if (!m_running.load()) break;
            continue;
        }

        handleClient(clientSocket);
        close(clientSocket);
    }
}

void HttpServer::handleClient(int clientSocket) {
    std::vector<char> buffer(65536);
    ssize_t bytesRead = read(clientSocket, buffer.data(), buffer.size() - 1);
    if (bytesRead <= 0) return;
    buffer[static_cast<size_t>(bytesRead)] = '\0';

    std::string_view req(buffer.data(), static_cast<size_t>(bytesRead));
    size_t lineEnd = req.find("\r\n");
    if (lineEnd == std::string_view::npos) return;

    std::string_view reqLine = req.substr(0, lineEnd);
    size_t mEnd = reqLine.find(' ');
    if (mEnd == std::string_view::npos) return;
    std::string_view method = reqLine.substr(0, mEnd);

    size_t uStart = mEnd + 1;
    size_t uEnd = reqLine.find(' ', uStart);
    if (uEnd == std::string_view::npos) return;
    std::string_view path = reqLine.substr(uStart, uEnd - uStart);

    size_t bodyPos = req.find("\r\n\r\n");
    std::string_view body = (bodyPos != std::string_view::npos) ? req.substr(bodyPos + 4) : "";

    std::string response = handleRequest(method, path, body);
    write(clientSocket, response.data(), response.size());
}

std::string HttpServer::handleRequest(std::string_view method, std::string_view path, std::string_view body) {
    // Health check
    if (path == "/health") {
        std::string json = "{\"status\":\"healthy\",\"engine\":\"educational-browser-engine\",\"version\":\"0.1.0\"}";
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " +
               std::to_string(json.size()) + "\r\nConnection: close\r\n\r\n" + json;
    }

    // CORS preflight
    if (method == "OPTIONS") {
        return "HTTP/1.1 204 No Content\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\nConnection: close\r\n\r\n";
    }

    // Interactive Web Workbench UI
    if (path == "/" || path == "/index.html") {
        std::string html = getIndexHtml();
        return "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " +
               std::to_string(html.size()) + "\r\nConnection: close\r\n\r\n" + html;
    }

    // API: Navigate URL or load custom HTML
    if (path == "/api/navigate" && method == "POST") {
        std::string json = handleNavigate(body);
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " +
               std::to_string(json.size()) + "\r\nConnection: close\r\n\r\n" + json;
    }

    // API: Click on viewport coordinate
    if (path == "/api/click" && method == "POST") {
        std::string json = handleClick(body);
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " +
               std::to_string(json.size()) + "\r\nConnection: close\r\n\r\n" + json;
    }

    // API: Evaluate JavaScript
    if (path == "/api/eval" && method == "POST") {
        std::string json = handleEval(body);
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " +
               std::to_string(json.size()) + "\r\nConnection: close\r\n\r\n" + json;
    }

    // API: Diagnostics
    if (path == "/api/diagnostics" && method == "GET") {
        std::string json = core::Serializer::diagnosticsToJson();
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " +
               std::to_string(json.size()) + "\r\nConnection: close\r\n\r\n" + json;
    }

    // Not Found
    std::string err = "{\"error\":\"Not Found\"}";
    return "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\nContent-Length: " +
           std::to_string(err.size()) + "\r\nConnection: close\r\n\r\n" + err;
}

std::string HttpServer::buildEngineStateJson() {
    std::ostringstream ss;
    ss << "{";
    ss << "\"url\":\"" << core::Serializer::escapeJson(m_engine->currentUrl()) << "\",";

    // Vector SVG render
    std::string svg = m_engine->displayList().toSvg(
        static_cast<float>(m_config.viewportWidth),
        static_cast<float>(m_config.viewportHeight)
    );
    ss << "\"svg\":\"" << core::Serializer::escapeJson(svg) << "\",";

    // DOM tree
    if (m_engine->document()) {
        ss << "\"dom\":" << core::Serializer::domToJson(*m_engine->document()) << ",";
    } else {
        ss << "\"dom\":null,";
    }

    // Computed Styles
    ss << "\"styles\":" << core::Serializer::stylesToJson(m_engine->styleMap()) << ",";

    // Layout tree
    if (m_engine->rootLayoutBox()) {
        ss << "\"layout\":" << core::Serializer::layoutToJson(*m_engine->rootLayoutBox()) << ",";
    } else {
        ss << "\"layout\":null,";
    }

    // Display List
    ss << "\"displayList\":" << m_engine->displayList().toJson() << ",";

    // Diagnostics
    ss << "\"diagnostics\":" << core::Serializer::diagnosticsToJson();
    ss << "}";
    return ss.str();
}

std::string HttpServer::handleNavigate(std::string_view body) {
    std::string url = extractJsonString(body, "url");
    std::string html = extractJsonString(body, "html");

    if (!html.empty()) {
        std::string baseUrl = extractJsonString(body, "baseUrl");
        if (baseUrl.empty()) baseUrl = "custom://live-editor";
        m_engine->loadHtml(html, baseUrl);
    } else if (!url.empty()) {
        m_engine->loadUrl(url);
    } else {
        m_engine->loadUrl("about:blank");
    }

    return buildEngineStateJson();
}

std::string HttpServer::handleClick(std::string_view body) {
    int x = extractJsonInt(body, "x", 0);
    int y = extractJsonInt(body, "y", 0);

    // Browser chrome height is 44px in native window, so adjust to content viewport
    m_engine->dispatchClick(core::Point{x, y + 44});

    return buildEngineStateJson();
}

std::string HttpServer::handleEval(std::string_view body) {
    std::string code = extractJsonString(body, "code");
    std::string result = m_engine->evaluateJavaScript(code);

    std::ostringstream ss;
    ss << "{";
    ss << "\"result\":\"" << core::Serializer::escapeJson(result) << "\",";
    ss << "\"state\":" << buildEngineStateJson();
    ss << "}";
    return ss.str();
}

std::string HttpServer::getIndexHtml() {
    return R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Browser Engine Workbench</title>
  <style>
    :root {
      --bg: #faf8f5;
      --card-bg: #ffffff;
      --border: #e8e3d8;
      --text-main: #38332d;
      --text-muted: #7c756c;
      --accent-sage: #daebd8;
      --accent-sage-dark: #446b4e;
      --accent-lavender: #eae3f4;
      --accent-lavender-dark: #5f527c;
      --accent-blue: #edf2f9;
      --accent-blue-dark: #5a73a0;
      --radius-sm: 8px;
      --radius-md: 14px;
      --radius-lg: 20px;
      --radius-pill: 24px;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: ui-sans-serif, system-ui, -apple-system, sans-serif;
      background-color: var(--bg);
      color: var(--text-main);
      display: flex;
      flex-direction: column;
      height: 100vh;
      overflow: hidden;
    }
    header {
      background-color: var(--card-bg);
      border-bottom: 1px solid var(--border);
      padding: 10px 24px;
      display: flex;
      align-items: center;
      justify-content: space-between;
    }
    .brand {
      display: flex;
      align-items: center;
      gap: 12px;
    }
    .brand-logo {
      width: 28px;
      height: 28px;
      background: linear-gradient(135deg, #76a880, #5a73a0);
      border-radius: 8px;
    }
    .brand-title {
      font-size: 17px;
      font-weight: 700;
      color: var(--text-main);
    }
    .status-badge {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      background-color: var(--accent-sage);
      color: var(--accent-sage-dark);
      padding: 4px 12px;
      border-radius: var(--radius-pill);
      font-size: 12px;
      font-weight: 600;
    }
    .status-dot {
      width: 8px;
      height: 8px;
      background-color: #446b4e;
      border-radius: 50%;
    }
    .main-container {
      display: flex;
      flex: 1;
      overflow: hidden;
    }
    .viewport-column {
      flex: 1;
      display: flex;
      flex-direction: column;
      border-right: 1px solid var(--border);
      background-color: var(--bg);
    }
    .toolbar {
      padding: 10px 16px;
      display: flex;
      align-items: center;
      gap: 10px;
      background-color: var(--card-bg);
      border-bottom: 1px solid var(--border);
    }
    .nav-btn {
      background: #f6f3ed;
      border: 1px solid var(--border);
      color: var(--text-main);
      border-radius: var(--radius-sm);
      width: 32px;
      height: 32px;
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      font-weight: bold;
    }
    .nav-btn:hover { background: #ede8dc; }
    .url-input-container {
      flex: 1;
      display: flex;
      align-items: center;
      background: #ffffff;
      border: 1px solid var(--border);
      border-radius: var(--radius-pill);
      padding: 0 14px;
      height: 36px;
    }
    .url-input {
      width: 100%;
      border: none;
      outline: none;
      font-size: 14px;
      color: var(--text-main);
      background: transparent;
    }
    .go-btn {
      background-color: #5a73a0;
      color: #ffffff;
      border: none;
      border-radius: var(--radius-pill);
      padding: 6px 16px;
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
    }
    .preset-bar {
      display: flex;
      gap: 8px;
      padding: 8px 16px;
      background: #fdfcf9;
      border-bottom: 1px solid var(--border);
      overflow-x: auto;
    }
    .preset-pill {
      background: #ffffff;
      border: 1px solid var(--border);
      border-radius: var(--radius-pill);
      padding: 4px 12px;
      font-size: 12px;
      color: var(--text-muted);
      cursor: pointer;
      white-space: nowrap;
    }
    .preset-pill:hover {
      background: var(--accent-blue);
      color: var(--accent-blue-dark);
      border-color: #d1dced;
    }
    .viewport-canvas-wrap {
      flex: 1;
      position: relative;
      overflow: auto;
      padding: 20px;
      display: flex;
      justify-content: center;
      align-items: flex-start;
    }
    .viewport-card {
      background: #ffffff;
      border: 1px solid var(--border);
      border-radius: var(--radius-lg);
      box-shadow: 0 4px 20px rgba(0,0,0,0.04);
      overflow: hidden;
      position: relative;
      cursor: crosshair;
    }
    .viewport-coords {
      position: absolute;
      bottom: 12px;
      right: 12px;
      background: rgba(255, 255, 255, 0.85);
      border: 1px solid var(--border);
      border-radius: var(--radius-pill);
      padding: 4px 10px;
      font-size: 11px;
      font-family: monospace;
      color: var(--text-muted);
      pointer-events: none;
    }
    /* Inspector Column */
    .inspector-column {
      width: 480px;
      display: flex;
      flex-direction: column;
      background: var(--card-bg);
    }
    .tabs {
      display: flex;
      border-bottom: 1px solid var(--border);
      background: #faf8f4;
    }
    .tab {
      padding: 10px 14px;
      font-size: 13px;
      font-weight: 600;
      color: var(--text-muted);
      cursor: pointer;
      border-bottom: 2px solid transparent;
    }
    .tab.active {
      color: var(--text-main);
      border-bottom-color: #5a73a0;
      background: #ffffff;
    }
    .tab-content {
      flex: 1;
      overflow-y: auto;
      padding: 16px;
      font-size: 13px;
    }
    .tree-node {
      margin-left: 14px;
      line-height: 1.6;
      font-family: monospace;
    }
    .tag-badge {
      background: var(--accent-blue);
      color: var(--accent-blue-dark);
      padding: 2px 6px;
      border-radius: 4px;
      font-weight: bold;
    }
    .attr-key { color: #8e5838; }
    .attr-val { color: #43694f; }
    .repl-output {
      background: #fdfbf7;
      border: 1px solid var(--border);
      border-radius: var(--radius-md);
      padding: 12px;
      height: 320px;
      overflow-y: auto;
      font-family: monospace;
      margin-bottom: 12px;
    }
    .repl-input-box {
      display: flex;
      gap: 8px;
    }
    .repl-input {
      flex: 1;
      border: 1px solid var(--border);
      border-radius: var(--radius-pill);
      padding: 8px 14px;
      font-family: monospace;
      font-size: 13px;
      outline: none;
    }
    .data-table {
      width: 100%;
      border-collapse: collapse;
      font-size: 12px;
    }
    .data-table th, .data-table td {
      border: 1px solid var(--border);
      padding: 6px 10px;
      text-align: left;
    }
    .data-table th { background: #f9f7f2; color: var(--text-muted); }
  </style>
</head>
<body>
  <header>
    <div class="brand">
      <div class="brand-logo"></div>
      <div class="brand-title">Educational Browser Engine Workbench</div>
    </div>
    <div class="status-badge">
      <div class="status-dot"></div>
      C++20 Engine Live
    </div>
  </header>

  <div class="main-container">
    <div class="viewport-column">
      <div class="toolbar">
        <button class="nav-btn" onclick="navigate('examples/basic/index.html')">&#8592;</button>
        <button class="nav-btn" onclick="reload()">&#8635;</button>
        <div class="url-input-container">
          <input id="urlInput" class="url-input" value="about:blank" onkeydown="if(event.key==='Enter') navigate(this.value)">
        </div>
        <button class="go-btn" onclick="navigate(document.getElementById('urlInput').value)">Load</button>
      </div>

      <div class="preset-bar">
        <div class="preset-pill" onclick="navigate('about:blank')">Tab: Blank</div>
        <div class="preset-pill" onclick="navigate('examples/basic/index.html')">Demo: Basic Flow</div>
        <div class="preset-pill" onclick="navigate('examples/css/index.html')">Demo: CSS Rounded Cards</div>
        <div class="preset-pill" onclick="navigate('examples/scripts/index.html')">Demo: Interactive JS</div>
        <div class="preset-pill" onclick="navigate('examples/images/index.html')">Demo: Images</div>
        <div class="preset-pill" onclick="toggleHtmlModal()">Live HTML Editor</div>
      </div>

      <div class="viewport-canvas-wrap">
        <div id="viewportCard" class="viewport-card" onclick="handleViewportClick(event)" onmousemove="updateCoords(event)">
          <div id="svgContainer"></div>
          <div id="coordDisplay" class="viewport-coords">X: 0, Y: 0</div>
        </div>
      </div>
    </div>

    <div class="inspector-column">
      <div class="tabs">
        <div class="tab active" onclick="switchTab('dom', this)">DOM</div>
        <div class="tab" onclick="switchTab('styles', this)">Styles</div>
        <div class="tab" onclick="switchTab('layout', this)">Layout</div>
        <div class="tab" onclick="switchTab('display', this)">Display List</div>
        <div class="tab" onclick="switchTab('console', this)">JS REPL</div>
        <div class="tab" onclick="switchTab('diag', this)">Telemetry</div>
      </div>

      <div id="tabContent" class="tab-content">
        <!-- Dynamic Inspector Content -->
      </div>
    </div>
  </div>

  <script>
    let currentState = null;
    let currentTab = 'dom';

    async function navigate(target) {
      document.getElementById('urlInput').value = target;
      try {
        const res = await fetch('/api/navigate', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({url: target})
        });
        currentState = await res.json();
        renderState();
      } catch (e) {
        console.error("Navigation error:", e);
      }
    }

    async function handleViewportClick(ev) {
      const rect = ev.currentTarget.getBoundingClientRect();
      const x = Math.round(ev.clientX - rect.left);
      const y = Math.round(ev.clientY - rect.top);
      try {
        const res = await fetch('/api/click', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({x, y})
        });
        currentState = await res.json();
        if (currentState.url) {
          document.getElementById('urlInput').value = currentState.url;
        }
        renderState();
      } catch (e) {
        console.error("Click error:", e);
      }
    }

    function updateCoords(ev) {
      const rect = ev.currentTarget.getBoundingClientRect();
      const x = Math.round(ev.clientX - rect.left);
      const y = Math.round(ev.clientY - rect.top);
      document.getElementById('coordDisplay').textContent = `X: ${x}, Y: ${y}`;
    }

    function reload() {
      navigate(document.getElementById('urlInput').value);
    }

    function renderState() {
      if (!currentState) return;
      if (currentState.svg) {
        document.getElementById('svgContainer').innerHTML = currentState.svg;
      }
      renderTab();
    }

    function switchTab(name, el) {
      document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
      el.classList.add('active');
      currentTab = name;
      renderTab();
    }

    function renderTab() {
      const container = document.getElementById('tabContent');
      if (!currentState) {
        container.innerHTML = '<p style="color:var(--text-muted)">Loading engine state...</p>';
        return;
      }

      if (currentTab === 'dom') {
        container.innerHTML = renderDomNode(currentState.dom);
      } else if (currentTab === 'styles') {
        container.innerHTML = renderStyles(currentState.styles);
      } else if (currentTab === 'layout') {
        container.innerHTML = renderLayout(currentState.layout);
      } else if (currentTab === 'display') {
        container.innerHTML = renderDisplayList(currentState.displayList);
      } else if (currentTab === 'console') {
        container.innerHTML = `
          <div id="replLog" class="repl-output">Engine JavaScript Console ready.\nType expressions to execute.</div>
          <div class="repl-input-box">
            <input id="replInput" class="repl-input" placeholder="e.g. document.title or 40 + 2" onkeydown="if(event.key==='Enter') executeJs()">
            <button class="go-btn" onclick="executeJs()">Run</button>
          </div>
        `;
      } else if (currentTab === 'diag') {
        container.innerHTML = renderDiagnostics(currentState.diagnostics);
      }
    }

    function renderDomNode(node) {
      if (!node) return '<em>Empty DOM</em>';
      let html = '<div class="tree-node">';
      if (node.nodeType === 'Element') {
        html += `<span class="tag-badge">&lt;${node.nodeName}&gt;</span>`;
        if (node.attributes) {
          for (const [k, v] of Object.entries(node.attributes)) {
            html += ` <span class="attr-key">${k}</span>=<span class="attr-val">"${v}"</span>`;
          }
        }
      } else if (node.nodeType === 'Text') {
        html += `<span style="color:#7c756c">"${node.text}"</span>`;
      }
      if (node.children && node.children.length > 0) {
        for (const child of node.children) {
          html += renderDomNode(child);
        }
      }
      html += '</div>';
      return html;
    }

    function renderStyles(styles) {
      if (!styles || !styles.length) return '<p>No computed styles.</p>';
      let h = '<table class="data-table"><tr><th>Element</th><th>Display</th><th>Color</th><th>Background</th><th>Radius</th></tr>';
      for (const s of styles) {
        let name = s.tag + (s.id ? '#' + s.id : '') + (s.class ? '.' + s.class : '');
        h += `<tr><td><strong>${name}</strong></td><td>${s.display}</td><td>${s.color}</td><td>${s.backgroundColor}</td><td>${s.borderRadius}px</td></tr>`;
      }
      h += '</table>';
      return h;
    }

    function renderLayout(box) {
      if (!box) return '<p>No layout tree.</p>';
      let h = `<div class="tree-node"><strong>${box.type}</strong> (${box.content.x}, ${box.content.y}, ${box.content.width}x${box.content.height})`;
      if (box.text) h += ` <em>"${box.text}"</em>`;
      if (box.children) {
        for (const c of box.children) h += renderLayout(c);
      }
      h += '</div>';
      return h;
    }

    function renderDisplayList(dl) {
      if (!dl || !dl.length) return '<p>No display items.</p>';
      let h = '<table class="data-table"><tr><th>#</th><th>Type</th><th>Geometry</th><th>Radius</th><th>Details</th></tr>';
      dl.forEach((it, idx) => {
        h += `<tr><td>${idx}</td><td><strong>${it.type}</strong></td><td>(${it.x||0}, ${it.y||0}) ${it.width||0}x${it.height||0}</td><td>${it.radius||0}px</td><td>${it.color||it.text||''}</td></tr>`;
      });
      h += '</table>';
      return h;
    }

    function renderDiagnostics(diag) {
      if (!diag || !diag.subsystems) return '<p>No diagnostics.</p>';
      let h = '<table class="data-table"><tr><th>Subsystem</th><th>Calls</th><th>Total (ms)</th><th>Avg (ms)</th></tr>';
      for (const s of diag.subsystems) {
        h += `<tr><td><strong>${s.name}</strong></td><td>${s.calls}</td><td>${s.totalMs.toFixed(3)}</td><td>${s.avgMs.toFixed(3)}</td></tr>`;
      }
      h += '</table>';
      return h;
    }

    async function executeJs() {
      const input = document.getElementById('replInput');
      const log = document.getElementById('replLog');
      const code = input.value.trim();
      if (!code) return;
      input.value = '';
      log.textContent += `\n> ${code}`;
      try {
        const res = await fetch('/api/eval', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({code})
        });
        const data = await res.json();
        log.textContent += `\n< ${data.result}`;
        log.scrollTop = log.scrollHeight;
        if (data.state) {
          currentState = data.state;
          if (currentState.svg) {
            document.getElementById('svgContainer').innerHTML = currentState.svg;
          }
        }
      } catch (e) {
        log.textContent += `\nError: ${e.message}`;
      }
    }

    function toggleHtmlModal() {
      const customHtml = prompt("Enter HTML to render in engine:", "<!DOCTYPE html><html><body><h1>Hello from Browser Engine!</h1><p>Crafted with love.</p></body></html>");
      if (customHtml) {
        fetch('/api/navigate', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({html: customHtml})
        }).then(r => r.json()).then(st => {
          currentState = st;
          renderState();
        });
      }
    }

    // Auto-load initial tab
    navigate('about:blank');
  </script>
</body>
</html>)HTML";
}

} // namespace browser::server
