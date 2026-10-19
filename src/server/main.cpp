#include "browser/server/http_server.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <string_view>
#include <vector>

namespace {

std::atomic<bool> g_shutdownRequested{false};

void signalHandler(int /*signum*/) {
    g_shutdownRequested.store(true);
}

void printUsage(std::string_view program) {
    std::cout << "Educational Browser Engine - Production Web Server\n";
    std::cout << "Usage: " << program << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -p, --port <port>       Port to listen on (default: 8080)\n";
    std::cout << "  -b, --bind <host>       Host interface to bind to (default: 0.0.0.0)\n";
    std::cout << "  -w, --width <pixels>    Engine viewport width (default: 1024)\n";
    std::cout << "  -H, --height <pixels>   Engine viewport height (default: 768)\n";
    std::cout << "  -v, --verbose           Enable verbose logging\n";
    std::cout << "  -h, --help              Show this help message and exit\n";
}

} // namespace

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    browser::server::ServerConfig config;
    std::vector<std::string_view> args(argv + 1, argv + argc);

    for (size_t i = 0; i < args.size(); ++i) {
        auto arg = args[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if ((arg == "-p" || arg == "--port") && i + 1 < args.size()) {
            config.port = static_cast<uint16_t>(std::stoi(std::string(args[++i])));
        } else if ((arg == "-b" || arg == "--bind") && i + 1 < args.size()) {
            config.host = std::string(args[++i]);
        } else if ((arg == "-w" || arg == "--width") && i + 1 < args.size()) {
            config.viewportWidth = static_cast<uint32_t>(std::stoi(std::string(args[++i])));
        } else if ((arg == "-H" || arg == "--height") && i + 1 < args.size()) {
            config.viewportHeight = static_cast<uint32_t>(std::stoi(std::string(args[++i])));
        } else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    std::cout << "========================================================\n";
    std::cout << "    EDUCATIONAL BROWSER ENGINE - PRODUCTION SERVER      \n";
    std::cout << "========================================================\n";
    std::cout << "Starting HTTP server on " << config.host << ":" << config.port << " ...\n";
    std::cout << "Open your browser at: http://localhost:" << config.port << "\n";
    std::cout << "Press Ctrl+C to stop.\n\n";

    browser::server::HttpServer server(config);
    if (!server.start()) {
        std::cerr << "Failed to start HTTP server on port " << config.port << "\n";
        return 1;
    }

    while (!g_shutdownRequested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\nShutting down server gracefully...\n";
    server.stop();
    std::cout << "Server stopped. Goodbye!\n";
    return 0;
}

