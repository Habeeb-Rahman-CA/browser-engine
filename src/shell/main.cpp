#include "browser/shell/engine.hpp"
#include "browser/core/diagnostics.hpp"
#include "browser/html/tokenizer.hpp"
#include "browser/html/parser.hpp"
#include "browser/style/style_resolver.hpp"
#include "browser/css/css_parser.hpp"
#include "browser/layout/layout_tree_builder.hpp"
#include "browser/layout/layout_engine.hpp"
#include "browser/graphics/display_list_builder.hpp"
#include "browser/javascript/interpreter.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string_view>
#include <vector>
#include <filesystem>

namespace {

std::string readFile(std::string_view path) {
    std::ifstream file(std::string(path), std::ios::in | std::ios::binary);
    if (!file) {
        return std::string(path); // treat as raw HTML string if file cannot be opened
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void loadStylesWithLinks(std::string_view filePath, const browser::dom::Document& doc, browser::style::StyleResolver& resolver) {
    resolver.loadStylesFromDocument(doc);
    std::filesystem::path p(filePath);
    if (std::filesystem::exists(p)) {
        std::filesystem::path basePath = p.parent_path();
        for (const auto& elem : doc.getElementsByTagName("link")) {
            if (elem->getAttribute("rel") == "stylesheet" && elem->hasAttribute("href")) {
                std::string href = elem->getAttribute("href");
                std::filesystem::path cssPath = basePath.empty() ? std::filesystem::path(href) : basePath / href;
                if (std::filesystem::exists(cssPath)) {
                    resolver.addAuthorStyleSheet(browser::css::CssParser::parse(readFile(cssPath.string())));
                }
            }
        }
    }
}

void printUsage(std::string_view programName) {
    std::cout << "Educational Browser Engine\n";
    std::cout << "Usage: " << programName << " [options] [url-or-file]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help            Show this help message and exit\n";
    std::cout << "  -v, --version         Show version information\n";
    std::cout << "  --dump-tokens <file>  Tokenize HTML file and print token stream\n";
    std::cout << "  --dump-dom <file>     Parse HTML file and print DOM tree\n";
    std::cout << "  --dump-style <file>   Parse HTML/CSS and print computed styles\n";
    std::cout << "  --dump-layout <file>  Compute and print layout geometry tree\n";
    std::cout << "  --dump-display-list <file> Print display list paint commands\n";
    std::cout << "  --eval-js <code>      Evaluate JavaScript snippet and print result\n";
    std::cout << "  --headless            Run in headless mode without graphical display\n";
    std::cout << "  --frames <N>          Run for N frames and then exit (useful for testing/CI)\n";
    std::cout << "  --width <pixels>      Initial window width (default: 1024)\n";
    std::cout << "  --height <pixels>     Initial window height (default: 768)\n";
    std::cout << "  --verbose             Enable verbose debug and trace logging\n";
    std::cout << "  --no-diagnostics      Suppress diagnostics summary table on exit\n";
    std::cout << "\nControls:\n";
    std::cout << "  Escape                Quit the browser\n";
    std::cout << "  D                     Dump current diagnostics and profiling table\n";
}

} // namespace

int main(int argc, char* argv[]) {
    browser::shell::EngineConfig config;
    std::vector<std::string_view> args(argv + 1, argv + argc);

    for (size_t i = 0; i < args.size(); ++i) {
        auto arg = args[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "BrowserEngine version 0.1.0\n";
            return 0;
        } else if (arg == "--dump-tokens" && i + 1 < args.size()) {
            std::string content = readFile(args[++i]);
            browser::html::HtmlTokenizer tokenizer(content);
            auto tokens = tokenizer.tokenizeAll();
            std::cout << "\n=== HTML TOKEN STREAM (" << tokens.size() << " tokens) ===\n";
            for (const auto& tok : tokens) {
                std::cout << "  " << tok.toString() << "\n";
            }
            browser::core::BrowserDiagnostics::instance().dumpSummary(std::cout);
            return 0;
        } else if (arg == "--dump-dom" && i + 1 < args.size()) {
            std::string content = readFile(args[++i]);
            auto doc = browser::html::HtmlParser::parseDocument(content);
            std::cout << "\n=== DOM TREE INSPECTION ===\n";
            doc->dumpTree(std::cout);
            browser::core::BrowserDiagnostics::instance().dumpSummary(std::cout);
            return 0;
        } else if (arg == "--dump-style" && i + 1 < args.size()) {
            std::string filePath = std::string(args[++i]);
            std::string content = readFile(filePath);
            auto doc = browser::html::HtmlParser::parseDocument(content);
            browser::style::StyleResolver resolver;
            loadStylesWithLinks(filePath, *doc, resolver);
            std::unordered_map<const browser::dom::Node*, std::shared_ptr<browser::style::ComputedStyle>> styleMap;
            resolver.resolveTree(doc, styleMap);

            std::cout << "\n=== COMPUTED STYLES INSPECTION ===\n";
            for (const auto& [node, style] : styleMap) {
                if (node->nodeType() == browser::dom::NodeType::Element) {
                    const auto* el = static_cast<const browser::dom::Element*>(node);
                    std::cout << "<" << el->tagName();
                    if (!el->id().empty()) std::cout << " id=\"" << el->id() << "\"";
                    if (!el->className().empty()) std::cout << " class=\"" << el->className() << "\"";
                    std::cout << ">:\n";
                    std::cout << "    display: " << style->display << "\n";
                    std::cout << "    color: rgba(" << int(style->color.r) << ", " << int(style->color.g)
                              << ", " << int(style->color.b) << ", " << (style->color.a / 255.0f) << ")\n";
                    std::cout << "    background: rgba(" << int(style->backgroundColor.r) << ", " << int(style->backgroundColor.g)
                              << ", " << int(style->backgroundColor.b) << ", " << (style->backgroundColor.a / 255.0f) << ")\n";
                    std::cout << "    font-size: " << style->fontSize << "px, weight: " << style->fontWeight << "\n";
                    std::cout << "    margin: (" << style->margin.top.value << "px, " << style->margin.right.value
                              << "px, " << style->margin.bottom.value << "px, " << style->margin.left.value << "px)\n";
                    std::cout << "    padding: (" << style->padding.top.value << "px, " << style->padding.right.value
                              << "px, " << style->padding.bottom.value << "px, " << style->padding.left.value << "px)\n";
                    if (style->borderRadius > 0.0f) {
                        std::cout << "    border-radius: " << style->borderRadius << "px\n";
                    }
                }
            }
            browser::core::BrowserDiagnostics::instance().dumpSummary(std::cout);
            return 0;
        } else if (arg == "--dump-layout" && i + 1 < args.size()) {
            std::string filePath = std::string(args[++i]);
            std::string content = readFile(filePath);
            auto doc = browser::html::HtmlParser::parseDocument(content);
            browser::style::StyleResolver resolver;
            loadStylesWithLinks(filePath, *doc, resolver);
            std::unordered_map<const browser::dom::Node*, std::shared_ptr<browser::style::ComputedStyle>> styleMap;
            resolver.resolveTree(doc, styleMap);

            auto rootBox = browser::layout::LayoutTreeBuilder::build(doc, styleMap);
            if (rootBox) {
                browser::layout::LayoutEngine::layout(*rootBox, browser::core::Size{1024, 768});
                std::cout << "\n=== LAYOUT TREE GEOMETRY (Viewport: 1024x768) ===\n";
                rootBox->dumpLayoutTree(std::cout);
            }
            browser::core::BrowserDiagnostics::instance().dumpSummary(std::cout);
            return 0;
        } else if (arg == "--dump-display-list" && i + 1 < args.size()) {
            std::string filePath = std::string(args[++i]);
            std::string content = readFile(filePath);
            auto doc = browser::html::HtmlParser::parseDocument(content);
            browser::style::StyleResolver resolver;
            loadStylesWithLinks(filePath, *doc, resolver);
            std::unordered_map<const browser::dom::Node*, std::shared_ptr<browser::style::ComputedStyle>> styleMap;
            resolver.resolveTree(doc, styleMap);

            auto rootBox = browser::layout::LayoutTreeBuilder::build(doc, styleMap);
            if (rootBox) {
                browser::layout::LayoutEngine::layout(*rootBox, browser::core::Size{1024, 768});
                auto dl = browser::graphics::DisplayListBuilder::build(*rootBox);
                std::cout << "\n=== DISPLAY LIST PAINT COMMANDS (" << dl.size() << " items) ===\n";
                dl.dump(std::cout);
            }
            browser::core::BrowserDiagnostics::instance().dumpSummary(std::cout);
            return 0;
        } else if (arg == "--eval-js" && i + 1 < args.size()) {
            std::string script = std::string(args[++i]);
            browser::javascript::Interpreter interp;
            auto res = interp.evaluate(script);
            std::cout << "Result: " << res.toString() << "\n";
            browser::core::BrowserDiagnostics::instance().dumpSummary(std::cout);
            return 0;
        } else if (arg == "--headless") {
            config.window.headless = true;
        } else if (arg == "--verbose") {
            config.verbose = true;
        } else if (arg == "--no-diagnostics") {
            config.dumpDiagnosticsOnExit = false;
        } else if (arg == "--width" && i + 1 < args.size()) {
            config.window.width = std::stoi(std::string(args[++i]));
        } else if (arg == "--height" && i + 1 < args.size()) {
            config.window.height = std::stoi(std::string(args[++i]));
        } else if (arg == "--frames" && i + 1 < args.size()) {
            config.maxFrames = std::stoull(std::string(args[++i]));
        } else if (!arg.starts_with("-")) {
            config.initialUrl = std::string(arg);
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    browser::shell::BrowserEngine engine(config);
    return engine.run();
}

