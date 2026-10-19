#pragma once

#include "browser/shell/engine.hpp"
#include <string>
#include <cstdint>
#include <functional>
#include <atomic>
#include <thread>
#include <memory>

namespace browser::server {

struct ServerConfig {
    std::string host{"0.0.0.0"};
    uint16_t port{8080};
    uint32_t viewportWidth{1024};
    uint32_t viewportHeight{768};
    bool verbose{false};
};

class HttpServer {
public:
    explicit HttpServer(ServerConfig config);
    ~HttpServer();

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    // Start server (blocking or non-blocking)
    bool start();
    void stop();

    [[nodiscard]] bool isRunning() const noexcept { return m_running.load(); }
    [[nodiscard]] uint16_t port() const noexcept { return m_config.port; }

private:
    void runServerLoop();
    void handleClient(int clientSocket);

    // Request handlers
    std::string handleRequest(std::string_view method, std::string_view path, std::string_view body);
    std::string handleNavigate(std::string_view body);
    std::string handleClick(std::string_view body);
    std::string handleEval(std::string_view body);
    std::string getIndexHtml();

    // Helper: build full state payload
    std::string buildEngineStateJson();

    ServerConfig m_config;
    std::atomic<bool> m_running{false};
    int m_serverFd{-1};
    std::unique_ptr<shell::BrowserEngine> m_engine;
    std::thread m_serverThread;
};

} // namespace browser::server

