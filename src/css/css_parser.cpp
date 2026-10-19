#include "browser/css/css_parser.hpp"
#include "browser/core/diagnostics.hpp"

#include <algorithm>
#include <cctype>

namespace browser::css {

namespace {

static const CssToken s_eofToken{CssTokenType::EndOfFile, "", 0.0, ""};

std::string toLower(std::string_view str) {
    std::string res;
    res.reserve(str.size());
    for (char c : str) res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return res;
}

} // namespace

CssParser::CssParser(std::string_view input) {
    CssTokenizer tokenizer(input);
    m_tokens = tokenizer.tokenizeAll();
}

StyleSheet CssParser::parse(std::string_view css) {
    CssParser parser(css);
    return parser.parseStyleSheet();
}

std::vector<Declaration> CssParser::parseInline(std::string_view style) {
    CssParser parser(style);
    return parser.parseDeclarationList();
}

const CssToken& CssParser::peek(size_t offset) const noexcept {
    if (m_index + offset >= m_tokens.size()) {
        return s_eofToken;
    }
    return m_tokens[m_index + offset];
}

const CssToken& CssParser::advance() noexcept {
    if (m_index >= m_tokens.size()) {
        return s_eofToken;
    }
    return m_tokens[m_index++];
}

void CssParser::skipWhitespace() {
    while (peek().type == CssTokenType::Whitespace) {
        advance();
    }
}

bool CssParser::match(CssTokenType type) noexcept {
    skipWhitespace();
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

StyleSheet CssParser::parseStyleSheet() {
    core::ScopedTimer timer(core::Subsystem::CssParser);
    StyleSheet sheet;

    while (peek().type != CssTokenType::EndOfFile) {
        skipWhitespace();
        if (peek().type == CssTokenType::EndOfFile) break;

        // Skip @-rules for now (e.g. @charset, @media)
        if (peek().type == CssTokenType::Delim && peek().text == "@") {
            while (peek().type != CssTokenType::EndOfFile && peek().type != CssTokenType::Semicolon && peek().type != CssTokenType::CloseBrace) {
                advance();
            }
            if (peek().type == CssTokenType::Semicolon || peek().type == CssTokenType::CloseBrace) advance();
            continue;
        }

        auto selectors = parseSelectorList();
        if (selectors.empty()) {
            advance();
            continue;
        }

        skipWhitespace();
        if (match(CssTokenType::OpenBrace)) {
            auto decls = parseBlockDeclarations();
            core::BrowserDiagnostics::instance().incrementCounter("CssRulesParsed");
            core::BrowserDiagnostics::instance().incrementCounter("CssDeclarationsParsed", decls.size());
            sheet.rules.push_back(Rule{std::move(selectors), std::move(decls)});
        }
    }

    return sheet;
}

std::vector<ComplexSelector> CssParser::parseSelectorList() {
    std::vector<ComplexSelector> selectors;

    while (peek().type != CssTokenType::EndOfFile && peek().type != CssTokenType::OpenBrace) {
        auto sel = parseComplexSelector();
        if (!sel.parts.empty()) {
            selectors.push_back(std::move(sel));
        }

        skipWhitespace();
        if (peek().type == CssTokenType::Comma) {
            advance(); // consume ','
        } else {
            break;
        }
    }

    return selectors;
}

ComplexSelector CssParser::parseComplexSelector() {
    ComplexSelector complexSel;

    while (peek().type != CssTokenType::EndOfFile && peek().type != CssTokenType::OpenBrace && peek().type != CssTokenType::Comma) {
        skipWhitespace();
        auto compound = parseCompoundSelector();
        if (compound.simpleSelectors.empty()) {
            break;
        }

        Combinator comb = Combinator::None;
        if (peek().type == CssTokenType::Whitespace) {
            comb = Combinator::Descendant;
            skipWhitespace();
        }

        if (peek().type == CssTokenType::Delim && peek().text == ">") {
            advance();
            comb = Combinator::Child;
            skipWhitespace();
        }

        complexSel.parts.push_back(SelectorPart{std::move(compound), comb});
    }

    return complexSel;
}

CompoundSelector CssParser::parseCompoundSelector() {
    CompoundSelector compound;

    while (peek().type != CssTokenType::EndOfFile) {
        const auto& tok = peek();

        if (tok.type == CssTokenType::Ident) {
            // Type selector (e.g. div, h1)
            compound.simpleSelectors.push_back(SimpleSelector{SimpleSelectorType::Type, toLower(advance().text)});
        } else if (tok.type == CssTokenType::Delim && tok.text == "*") {
            advance();
            compound.simpleSelectors.push_back(SimpleSelector{SimpleSelectorType::Universal, "*"});
        } else if (tok.type == CssTokenType::Delim && tok.text == ".") {
            // Class selector (e.g. .card)
            advance(); // '.'
            if (peek().type == CssTokenType::Ident) {
                compound.simpleSelectors.push_back(SimpleSelector{SimpleSelectorType::Class, advance().text});
            }
        } else if (tok.type == CssTokenType::Hash) {
            // ID selector (e.g. #header)
            compound.simpleSelectors.push_back(SimpleSelector{SimpleSelectorType::Id, advance().text});
        } else if (tok.type == CssTokenType::Colon) {
            // Pseudo-class (e.g. :hover, :focus)
            advance(); // ':'
            if (peek().type == CssTokenType::Ident) {
                compound.simpleSelectors.push_back(SimpleSelector{SimpleSelectorType::PseudoClass, toLower(advance().text)});
            }
        } else {
            break;
        }
    }

    return compound;
}

std::vector<Declaration> CssParser::parseBlockDeclarations() {
    std::vector<Declaration> declarations;

    while (peek().type != CssTokenType::EndOfFile && peek().type != CssTokenType::CloseBrace) {
        skipWhitespace();
        if (peek().type == CssTokenType::CloseBrace) break;

        auto decl = parseDeclaration();
        if (decl) {
            declarations.push_back(std::move(*decl));
        }

        skipWhitespace();
        if (peek().type == CssTokenType::Semicolon) {
            advance();
        }
    }

    if (peek().type == CssTokenType::CloseBrace) {
        advance(); // consume '}'
    }

    return declarations;
}

std::vector<Declaration> CssParser::parseDeclarationList() {
    std::vector<Declaration> decls;
    while (peek().type != CssTokenType::EndOfFile) {
        skipWhitespace();
        if (peek().type == CssTokenType::EndOfFile) break;

        auto decl = parseDeclaration();
        if (decl) {
            decls.push_back(std::move(*decl));
        }

        skipWhitespace();
        if (peek().type == CssTokenType::Semicolon) {
            advance();
        }
    }
    return decls;
}

std::optional<Declaration> CssParser::parseDeclaration() {
    skipWhitespace();
    if (peek().type != CssTokenType::Ident) {
        // Recover to next semicolon or close brace
        while (peek().type != CssTokenType::EndOfFile && peek().type != CssTokenType::Semicolon && peek().type != CssTokenType::CloseBrace) {
            advance();
        }
        return std::nullopt;
    }

    std::string property = toLower(advance().text);

    skipWhitespace();
    if (!match(CssTokenType::Colon)) {
        return std::nullopt;
    }

    // Accumulate value tokens until ';' or '}'
    std::string value;
    bool important = false;

    while (peek().type != CssTokenType::EndOfFile && peek().type != CssTokenType::Semicolon && peek().type != CssTokenType::CloseBrace) {
        const auto& tok = peek();
        if (tok.type == CssTokenType::Delim && tok.text == "!") {
            advance();
            skipWhitespace();
            if (peek().type == CssTokenType::Ident && toLower(peek().text) == "important") {
                important = true;
                advance();
                continue;
            }
        }

        if (tok.type == CssTokenType::Whitespace) {
            advance();
            if (!value.empty() && value.back() != ' ') {
                value.push_back(' ');
            }
            continue;
        }

        if (tok.type == CssTokenType::Hash) {
            value += "#" + tok.text;
        } else {
            value += tok.text;
        }
        advance();
    }

    // Trim leading/trailing whitespace
    size_t start = value.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) {
        size_t end = value.find_last_not_of(" \t\r\n");
        value = value.substr(start, end - start + 1);
    } else {
        value.clear();
    }

    return Declaration{property, value, important};
}

} // namespace browser::css
