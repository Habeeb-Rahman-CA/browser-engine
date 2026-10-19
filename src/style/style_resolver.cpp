#include "browser/style/style_resolver.hpp"
#include "browser/style/selector_matcher.hpp"
#include "browser/css/css_parser.hpp"
#include "browser/core/diagnostics.hpp"

#include <algorithm>
#include <sstream>

namespace browser::style {

namespace {

struct MatchedRule {
    css::Declaration decl;
    uint8_t origin{0}; // 0 = UserAgent, 1 = Author, 2 = Inline
    css::Specificity specificity;
    size_t sourceIndex{0};

    bool operator<(const MatchedRule& other) const noexcept {
        // 1. Important flag
        if (decl.important != other.decl.important) {
            return decl.important < other.decl.important;
        }
        // 2. Origin
        if (origin != other.origin) {
            return origin < other.origin;
        }
        // 3. Specificity
        if (specificity != other.specificity) {
            return specificity < other.specificity;
        }
        // 4. Source order
        return sourceIndex < other.sourceIndex;
    }
};

void parseEdgeShorthand(std::string_view value, BoxEdges& edges) {
    std::istringstream ss{std::string(value)};
    std::vector<std::string> parts;
    std::string part;
    while (ss >> part) {
        parts.push_back(part);
    }

    if (parts.size() == 1) {
        auto len = ComputedStyle::parseLength(parts[0]);
        edges.top = edges.right = edges.bottom = edges.left = len;
    } else if (parts.size() == 2) {
        auto v = ComputedStyle::parseLength(parts[0]);
        auto h = ComputedStyle::parseLength(parts[1]);
        edges.top = edges.bottom = v;
        edges.right = edges.left = h;
    } else if (parts.size() == 3) {
        edges.top = ComputedStyle::parseLength(parts[0]);
        edges.right = edges.left = ComputedStyle::parseLength(parts[1]);
        edges.bottom = ComputedStyle::parseLength(parts[2]);
    } else if (parts.size() >= 4) {
        edges.top = ComputedStyle::parseLength(parts[0]);
        edges.right = ComputedStyle::parseLength(parts[1]);
        edges.bottom = ComputedStyle::parseLength(parts[2]);
        edges.left = ComputedStyle::parseLength(parts[3]);
    }
}

} // namespace

StyleResolver::StyleResolver() {
    initUserAgentStyleSheet();
}

void StyleResolver::initUserAgentStyleSheet() {
    static const char* s_defaultUaCss = R"(
        html, body { display: block; margin: 0; padding: 0; }
        head, title, meta, link, style, script { display: none; }
        div, section, article, header, footer, nav, main, aside { display: block; }
        h1 { display: block; font-size: 32px; font-weight: 700; margin-top: 21px; margin-bottom: 21px; }
        h2 { display: block; font-size: 24px; font-weight: 700; margin-top: 19px; margin-bottom: 19px; }
        h3 { display: block; font-size: 18px; font-weight: 700; margin-top: 18px; margin-bottom: 18px; }
        h4, h5, h6 { display: block; font-weight: 700; }
        p { display: block; margin-top: 16px; margin-bottom: 16px; }
        ul, ol { display: block; margin-top: 16px; margin-bottom: 16px; padding-left: 40px; }
        li { display: block; }
        strong, b { font-weight: 700; }
        a { color: blue; }
        img { display: inline-block; }
    )";
    m_userAgentSheet = css::CssParser::parse(s_defaultUaCss);
}

void StyleResolver::addAuthorStyleSheet(css::StyleSheet sheet) {
    m_authorSheets.push_back(std::move(sheet));
}

void StyleResolver::clearAuthorStyleSheets() {
    m_authorSheets.clear();
}

void StyleResolver::loadStylesFromDocument(const dom::Document& document) {
    clearAuthorStyleSheets();
    auto styleElements = document.getElementsByTagName("style");
    for (const auto& el : styleElements) {
        std::string cssText = el->textContent();
        if (!cssText.empty()) {
            addAuthorStyleSheet(css::CssParser::parse(cssText));
        }
    }
}

std::shared_ptr<ComputedStyle> StyleResolver::resolveStyle(
    const dom::Element& element,
    const ComputedStyle* parentStyle
) {
    core::ScopedTimer timer(core::Subsystem::StyleResolution);
    auto style = std::make_shared<ComputedStyle>();

    // 1. Inherit applicable properties from parent style
    if (parentStyle) {
        style->color = parentStyle->color;
        style->fontSize = parentStyle->fontSize;
        style->fontWeight = parentStyle->fontWeight;
        style->fontFamily = parentStyle->fontFamily;
        style->textAlign = parentStyle->textAlign;
    }

    std::vector<MatchedRule> matchedRules;
    size_t sourceCounter = 0;

    // 2. Collect matched declarations from User-Agent stylesheet (Origin = 0)
    for (const auto& rule : m_userAgentSheet.rules) {
        for (const auto& sel : rule.selectors) {
            if (SelectorMatcher::matches(element, sel)) {
                for (const auto& decl : rule.declarations) {
                    matchedRules.push_back({decl, 0, sel.specificity(), sourceCounter++});
                }
                break;
            }
        }
    }

    // 3. Collect matched declarations from Author stylesheets (Origin = 1)
    for (const auto& sheet : m_authorSheets) {
        for (const auto& rule : sheet.rules) {
            for (const auto& sel : rule.selectors) {
                if (SelectorMatcher::matches(element, sel)) {
                    for (const auto& decl : rule.declarations) {
                        matchedRules.push_back({decl, 1, sel.specificity(), sourceCounter++});
                    }
                    break;
                }
            }
        }
    }

    // 4. Collect inline styles from element's style attribute (Origin = 2, highest specificity)
    if (element.hasAttribute("style")) {
        auto inlineDecls = css::CssParser::parseInline(element.getAttribute("style"));
        css::Specificity inlineSpec{100, 0, 0}; // Highest specificity for inline style
        for (auto& decl : inlineDecls) {
            matchedRules.push_back({std::move(decl), 2, inlineSpec, sourceCounter++});
        }
    }

    // 5. Sort matches according to cascade order
    std::stable_sort(matchedRules.begin(), matchedRules.end());

    // 6. Apply matched declarations in priority order
    for (const auto& match : matchedRules) {
        applyDeclaration(*style, match.decl);
    }

    core::BrowserDiagnostics::instance().incrementCounter("StylesResolved");
    return style;
}

void StyleResolver::applyDeclaration(ComputedStyle& style, const css::Declaration& decl) {
    const auto& prop = decl.property;
    const auto& val = decl.value;

    if (prop == "display") {
        style.display = ComputedStyle::parseDisplay(val);
    } else if (prop == "color") {
        if (auto col = ComputedStyle::parseColor(val)) {
            style.color = *col;
        }
    } else if (prop == "background-color" || prop == "background") {
        if (auto col = ComputedStyle::parseColor(val)) {
            style.backgroundColor = *col;
        }
    } else if (prop == "font-size") {
        auto len = ComputedStyle::parseLength(val);
        if (len.isPx()) {
            style.fontSize = len.value;
        }
    } else if (prop == "font-weight") {
        if (val == "bold" || val == "700" || val == "800" || val == "900") {
            style.fontWeight = 700;
        } else if (val == "normal" || val == "400") {
            style.fontWeight = 400;
        }
    } else if (prop == "text-align") {
        style.textAlign = ComputedStyle::parseTextAlign(val);
    } else if (prop == "width") {
        style.width = ComputedStyle::parseLength(val);
    } else if (prop == "height") {
        style.height = ComputedStyle::parseLength(val);
    }
    // Margin
    else if (prop == "margin") {
        parseEdgeShorthand(val, style.margin);
    } else if (prop == "margin-top") {
        style.margin.top = ComputedStyle::parseLength(val);
    } else if (prop == "margin-right") {
        style.margin.right = ComputedStyle::parseLength(val);
    } else if (prop == "margin-bottom") {
        style.margin.bottom = ComputedStyle::parseLength(val);
    } else if (prop == "margin-left") {
        style.margin.left = ComputedStyle::parseLength(val);
    }
    // Padding
    else if (prop == "padding") {
        parseEdgeShorthand(val, style.padding);
    } else if (prop == "padding-top") {
        style.padding.top = ComputedStyle::parseLength(val);
    } else if (prop == "padding-right") {
        style.padding.right = ComputedStyle::parseLength(val);
    } else if (prop == "padding-bottom") {
        style.padding.bottom = ComputedStyle::parseLength(val);
    } else if (prop == "padding-left") {
        style.padding.left = ComputedStyle::parseLength(val);
    }
    // Border
    else if (prop == "border-width") {
        auto len = ComputedStyle::parseLength(val);
        float w = len.isPx() ? len.value : 0.0f;
        style.border.top.width = style.border.right.width = style.border.bottom.width = style.border.left.width = w;
    } else if (prop == "border-color") {
        if (auto col = ComputedStyle::parseColor(val)) {
            style.border.top.color = style.border.right.color = style.border.bottom.color = style.border.left.color = *col;
        }
    } else if (prop == "border") {
        // e.g. "1px solid black" or "2px #ff0000"
        std::istringstream ss{val};
        std::string token;
        while (ss >> token) {
            if (token == "solid") {
                style.border.top.style = style.border.right.style = style.border.bottom.style = style.border.left.style = BorderStyle::Solid;
            } else if (token == "none") {
                style.border.top.style = style.border.right.style = style.border.bottom.style = style.border.left.style = BorderStyle::None;
            } else if (auto col = ComputedStyle::parseColor(token)) {
                style.border.top.color = style.border.right.color = style.border.bottom.color = style.border.left.color = *col;
            } else {
                auto len = ComputedStyle::parseLength(token);
                if (len.isPx()) {
                    style.border.top.width = style.border.right.width = style.border.bottom.width = style.border.left.width = len.value;
                    style.border.top.style = style.border.right.style = style.border.bottom.style = style.border.left.style = BorderStyle::Solid;
                }
            }
        }
    } else if (prop == "opacity") {
        try {
            style.opacity = std::clamp(std::stof(val), 0.0f, 1.0f);
        } catch (...) {}
    } else if (prop == "border-radius") {
        auto len = ComputedStyle::parseLength(val);
        if (len.isPx()) {
            style.borderRadius = len.value;
        }
    }
}

void StyleResolver::resolveTree(
    const std::shared_ptr<dom::Node>& root,
    std::unordered_map<const dom::Node*, std::shared_ptr<ComputedStyle>>& styleMap,
    const ComputedStyle* parentStyle
) {
    if (!root) return;

    const ComputedStyle* currentComputed = parentStyle;

    if (root->nodeType() == dom::NodeType::Element) {
        auto elem = std::static_pointer_cast<dom::Element>(root);
        auto computed = resolveStyle(*elem, parentStyle);
        styleMap[root.get()] = computed;
        currentComputed = computed.get();
    }

    for (const auto& child : root->childNodes()) {
        resolveTree(child, styleMap, currentComputed);
    }
}

} // namespace browser::style

