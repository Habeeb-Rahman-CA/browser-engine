#include "browser/html/parser.hpp"
#include "browser/css/css_parser.hpp"
#include "browser/style/style_resolver.hpp"
#include "browser/layout/layout_tree_builder.hpp"
#include "browser/layout/layout_engine.hpp"
#include "browser/graphics/display_list_builder.hpp"
#include "browser/javascript/interpreter.hpp"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>

using namespace browser;

namespace {

const char* s_sampleHtml = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Benchmark Document</title>
    <style>
        body { margin: 0; padding: 20px; font-family: sans-serif; background-color: #fff; }
        .container { width: 800px; margin: 0 auto; padding: 15px; }
        h1 { color: #1a73e8; margin-bottom: 12px; }
        p { color: #3c4043; line-height: 1.5; margin-bottom: 10px; }
        .card { background-color: #f8f9fa; border: 1px solid #dadce0; padding: 16px; margin-bottom: 16px; }
        .card-header { font-weight: 700; color: #202124; margin-bottom: 8px; }
        .highlight { color: #d93025; font-weight: 700; }
        ul { padding-left: 20px; }
        li { margin-bottom: 4px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Browser Engine Benchmark</h1>
        <p class="intro">Measuring the performance of the full browser processing pipeline.</p>
        <div class="card">
            <div class="card-header">Component Alpha</div>
            <p>Testing layout, style cascading, and recursive box calculations.</p>
            <ul>
                <li>Item One: <span class="highlight">Active</span></li>
                <li>Item Two: Processing block elements</li>
                <li>Item Three: Inline text runs and spans</li>
            </ul>
        </div>
        <div class="card">
            <div class="card-header">Component Beta</div>
            <p>Second block with nested contents and margins.</p>
        </div>
    </div>
</body>
</html>
)";

void printBenchResult(std::string_view name, int iterations, double totalMs) {
    double avgUs = (totalMs * 1000.0) / iterations;
    double opsPerSec = (iterations / totalMs) * 1000.0;
    std::cout << std::left << std::setw(35) << name
              << std::right << std::setw(10) << iterations
              << std::setw(15) << std::fixed << std::setprecision(3) << totalMs
              << std::setw(15) << std::fixed << std::setprecision(2) << avgUs
              << std::setw(15) << std::fixed << std::setprecision(0) << opsPerSec
              << "\n";
}

} // namespace

int main() {
    std::cout << "========================================================================================\n";
    std::cout << "                           BROWSER ENGINE BENCHMARK SUITE                               \n";
    std::cout << "========================================================================================\n";
    std::cout << std::left << std::setw(35) << "Benchmark Name"
              << std::right << std::setw(10) << "Iters"
              << std::setw(15) << "Total (ms)"
              << std::setw(15) << "Avg (us)"
              << std::setw(15) << "Ops/sec"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    // 1. HTML Parsing Benchmark
    {
        const int iters = 2000;
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iters; ++i) {
            auto doc = html::HtmlParser::parseDocument(s_sampleHtml);
            (void)doc;
        }
        auto end = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        printBenchResult("HTML Tokenize & Parse", iters, ms);
    }

    // 2. CSS Parsing Benchmark
    {
        const std::string css = R"(
            body { margin: 0; padding: 20px; font-family: sans-serif; background-color: #fff; }
            .container { width: 800px; margin: 0 auto; padding: 15px; }
            h1 { color: #1a73e8; margin-bottom: 12px; }
            p { color: #3c4043; margin-bottom: 10px; }
            .card { background-color: #f8f9fa; border: 1px solid #dadce0; padding: 16px; margin-bottom: 16px; }
            .card-header { font-weight: 700; color: #202124; margin-bottom: 8px; }
            .highlight { color: #d93025; font-weight: 700; }
            ul { padding-left: 20px; }
            li { margin-bottom: 4px; }
        )";
        const int iters = 5000;
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iters; ++i) {
            auto sheet = css::CssParser::parse(css);
            (void)sheet;
        }
        auto end = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        printBenchResult("CSS Tokenize & Parse", iters, ms);
    }

    // 3. Style Resolution Benchmark
    {
        auto doc = html::HtmlParser::parseDocument(s_sampleHtml);
        style::StyleResolver resolver;
        resolver.loadStylesFromDocument(*doc);

        const int iters = 2000;
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iters; ++i) {
            std::unordered_map<const dom::Node*, std::shared_ptr<style::ComputedStyle>> styleMap;
            resolver.resolveTree(doc, styleMap);
        }
        auto end = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        printBenchResult("Style Resolution & Cascade", iters, ms);
    }

    // 4. Layout Computation Benchmark
    {
        auto doc = html::HtmlParser::parseDocument(s_sampleHtml);
        style::StyleResolver resolver;
        resolver.loadStylesFromDocument(*doc);
        std::unordered_map<const dom::Node*, std::shared_ptr<style::ComputedStyle>> styleMap;
        resolver.resolveTree(doc, styleMap);

        const int iters = 5000;
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iters; ++i) {
            auto rootBox = layout::LayoutTreeBuilder::build(doc, styleMap);
            if (rootBox) {
                layout::LayoutEngine::layout(*rootBox, core::Size{1024, 768});
            }
        }
        auto end = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        printBenchResult("Layout Tree & Box Model", iters, ms);
    }

    // 5. JavaScript Execution Benchmark
    {
        const std::string jsScript = R"(
            function fib(n) {
                let a = 0;
                let b = 1;
                for (let i = 0; i < n; i = i + 1) {
                    let temp = a + b;
                    a = b;
                    b = temp;
                }
                return a;
            }
            let res = fib(50);
        )";
        const int iters = 2000;
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iters; ++i) {
            javascript::Interpreter interp;
            interp.evaluate(jsScript);
        }
        auto end = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        printBenchResult("JavaScript Lex/Parse/Eval", iters, ms);
    }

    // 6. Full End-to-End Pipeline Benchmark
    {
        const int iters = 1000;
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < iters; ++i) {
            auto doc = html::HtmlParser::parseDocument(s_sampleHtml);
            style::StyleResolver resolver;
            resolver.loadStylesFromDocument(*doc);
            std::unordered_map<const dom::Node*, std::shared_ptr<style::ComputedStyle>> styleMap;
            resolver.resolveTree(doc, styleMap);
            auto rootBox = layout::LayoutTreeBuilder::build(doc, styleMap);
            if (rootBox) {
                layout::LayoutEngine::layout(*rootBox, core::Size{1024, 768});
                auto dl = graphics::DisplayListBuilder::build(*rootBox);
                (void)dl;
            }
        }
        auto end = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        printBenchResult("Full Pipeline (HTML->Paint)", iters, ms);
    }

    std::cout << "========================================================================================\n";
    return 0;
}

