#include "browser/html/tokenizer.hpp"
#include "browser/core/diagnostics.hpp"

#include <cctype>
#include <algorithm>
#include <charconv>

namespace browser::html {

namespace {

bool isWhitespace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f';
}

// Convert codepoint to UTF-8 bytes
std::string codepointToUtf8(uint32_t cp) {
    std::string out;
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return out;
}

} // namespace

HtmlTokenizer::HtmlTokenizer(std::string_view input)
    : m_input(input) {}

bool HtmlTokenizer::isVoidElement(std::string_view tagName) noexcept {
    static const std::string_view voidTags[] = {
        "area", "base", "br", "col", "embed", "hr", "img",
        "input", "link", "meta", "param", "source", "track", "wbr"
    };
    for (auto voidTag : voidTags) {
        if (tagName == voidTag) return true;
    }
    return false;
}

char HtmlTokenizer::peek(size_t offset) const noexcept {
    if (m_index + offset >= m_input.size()) {
        return '\0';
    }
    return m_input[m_index + offset];
}

char HtmlTokenizer::advance() noexcept {
    if (m_index >= m_input.size()) {
        return '\0';
    }
    char c = m_input[m_index++];
    if (c == '\n') {
        m_line++;
        m_column = 1;
    } else {
        m_column++;
    }
    return c;
}

bool HtmlTokenizer::match(std::string_view expected) noexcept {
    if (m_index + expected.size() > m_input.size()) {
        return false;
    }
    for (size_t i = 0; i < expected.size(); ++i) {
        char in = static_cast<char>(std::tolower(static_cast<unsigned char>(m_input[m_index + i])));
        char ex = static_cast<char>(std::tolower(static_cast<unsigned char>(expected[i])));
        if (in != ex) return false;
    }
    for (size_t i = 0; i < expected.size(); ++i) {
        advance();
    }
    return true;
}

void HtmlTokenizer::emitToken(HtmlToken token) {
    if (token.type == TokenType::StartTag) {
        // Auto-mark void elements as self-closing
        if (isVoidElement(token.tagName)) {
            token.selfClosing = true;
        }
    }
    core::BrowserDiagnostics::instance().incrementCounter("TokensEmitted");
    m_tokenQueue.push_back(std::move(token));
}

std::string HtmlTokenizer::consumeCharacterReference() {
    // We already peeked '&'
    advance(); // consume '&'

    std::string ref;
    // Check if numeric
    if (peek() == '#') {
        advance(); // consume '#'
        bool isHex = (peek() == 'x' || peek() == 'X');
        if (isHex) advance();

        uint32_t codepoint = 0;
        size_t count = 0;
        while (count < 8) {
            char c = peek();
            if (isHex) {
                if (std::isxdigit(static_cast<unsigned char>(c))) {
                    uint32_t digitVal = std::isdigit(static_cast<unsigned char>(c)) ?
                        static_cast<uint32_t>(c - '0') :
                        static_cast<uint32_t>(std::tolower(static_cast<unsigned char>(c)) - 'a' + 10);
                    codepoint = codepoint * 16u + digitVal;
                    advance();
                    count++;
                } else break;
            } else {
                if (std::isdigit(static_cast<unsigned char>(c))) {
                    codepoint = codepoint * 10u + static_cast<uint32_t>(c - '0');
                    advance();
                    count++;
                } else break;
            }
        }
        if (peek() == ';') advance();
        return codepointToUtf8(codepoint);
    }

    // Named entity
    std::string name;
    while (std::isalpha(static_cast<unsigned char>(peek())) && name.size() < 10) {
        name.push_back(advance());
    }
    if (peek() == ';') advance();

    if (name == "amp") return "&";
    if (name == "lt") return "<";
    if (name == "gt") return ">";
    if (name == "quot") return "\"";
    if (name == "apos") return "'";
    if (name == "nbsp") return "\u00A0";
    if (name == "copy") return "\u00A9";
    if (name == "reg") return "\u00AE";

    // Unrecognized, return literal string
    return "&" + name + ";";
}

HtmlToken HtmlTokenizer::nextToken() {
    core::ScopedTimer timer(core::Subsystem::HtmlTokenizer);

    while (m_tokenQueue.empty()) {
        if (m_index >= m_input.size()) {
            // End of input reached
            if (m_state == State::TagName || m_state == State::AttributeName || m_state == State::BeforeAttributeValue) {
                emitToken(m_currentToken);
                m_state = State::Data;
                continue;
            }
            HtmlToken eofToken;
            eofToken.type = TokenType::EndOfFile;
            eofToken.line = m_line;
            eofToken.column = m_column;
            return eofToken;
        }

        switch (m_state) {
            case State::Data: {
                char c = peek();
                if (c == '<') {
                    advance();
                    m_state = State::TagOpen;
                } else if (c == '&') {
                    std::string ref = consumeCharacterReference();
                    HtmlToken token;
                    token.type = TokenType::Character;
                    token.data = std::move(ref);
                    token.line = m_line;
                    token.column = m_column;
                    emitToken(std::move(token));
                } else {
                    // Accumulate character run
                    std::string chars;
                    while (m_index < m_input.size() && peek() != '<' && peek() != '&') {
                        chars.push_back(advance());
                    }
                    HtmlToken token;
                    token.type = TokenType::Character;
                    token.data = std::move(chars);
                    token.line = m_line;
                    token.column = m_column;
                    emitToken(std::move(token));
                }
                break;
            }

            case State::TagOpen: {
                char c = peek();
                if (c == '!') {
                    advance();
                    m_state = State::MarkupDeclarationOpen;
                } else if (c == '/') {
                    advance();
                    m_state = State::EndTagOpen;
                } else if (std::isalpha(static_cast<unsigned char>(c))) {
                    m_currentToken = HtmlToken{};
                    m_currentToken.type = TokenType::StartTag;
                    m_currentToken.line = m_line;
                    m_currentToken.column = m_column;
                    m_currentToken.tagName.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(advance()))));
                    m_state = State::TagName;
                } else if (c == '?') {
                    // Bogus comment
                    advance();
                    m_currentToken = HtmlToken{};
                    m_currentToken.type = TokenType::Comment;
                    m_state = State::BogusComment;
                } else {
                    // Malformed tag open, emit '<' as character
                    HtmlToken token;
                    token.type = TokenType::Character;
                    token.data = "<";
                    emitToken(std::move(token));
                    m_state = State::Data;
                }
                break;
            }

            case State::EndTagOpen: {
                char c = peek();
                if (std::isalpha(static_cast<unsigned char>(c))) {
                    m_currentToken = HtmlToken{};
                    m_currentToken.type = TokenType::EndTag;
                    m_currentToken.line = m_line;
                    m_currentToken.column = m_column;
                    m_currentToken.tagName.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(advance()))));
                    m_state = State::TagName;
                } else if (c == '>') {
                    advance();
                    m_state = State::Data;
                } else {
                    // Bogus comment recovery
                    m_currentToken = HtmlToken{};
                    m_currentToken.type = TokenType::Comment;
                    m_state = State::BogusComment;
                }
                break;
            }

            case State::TagName: {
                char c = peek();
                if (isWhitespace(c)) {
                    advance();
                    m_state = State::BeforeAttributeName;
                } else if (c == '/') {
                    advance();
                    m_state = State::SelfClosingStartTag;
                } else if (c == '>') {
                    advance();
                    emitToken(m_currentToken);
                    m_state = State::Data;
                } else {
                    m_currentToken.tagName.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(advance()))));
                }
                break;
            }

            case State::BeforeAttributeName: {
                char c = peek();
                if (isWhitespace(c)) {
                    advance();
                } else if (c == '/') {
                    advance();
                    m_state = State::SelfClosingStartTag;
                } else if (c == '>') {
                    advance();
                    emitToken(m_currentToken);
                    m_state = State::Data;
                } else {
                    m_currentAttrName.clear();
                    m_currentAttrVal.clear();
                    m_currentAttrName.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(advance()))));
                    m_state = State::AttributeName;
                }
                break;
            }

            case State::AttributeName: {
                char c = peek();
                if (isWhitespace(c)) {
                    advance();
                    m_state = State::AfterAttributeName;
                } else if (c == '=') {
                    advance();
                    m_state = State::BeforeAttributeValue;
                } else if (c == '/') {
                    // Flush boolean attribute
                    m_currentToken.attributes.push_back({m_currentAttrName, ""});
                    advance();
                    m_state = State::SelfClosingStartTag;
                } else if (c == '>') {
                    // Flush boolean attribute
                    m_currentToken.attributes.push_back({m_currentAttrName, ""});
                    advance();
                    emitToken(m_currentToken);
                    m_state = State::Data;
                } else {
                    m_currentAttrName.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(advance()))));
                }
                break;
            }

            case State::AfterAttributeName: {
                char c = peek();
                if (isWhitespace(c)) {
                    advance();
                } else if (c == '=') {
                    advance();
                    m_state = State::BeforeAttributeValue;
                } else if (c == '/') {
                    m_currentToken.attributes.push_back({m_currentAttrName, ""});
                    advance();
                    m_state = State::SelfClosingStartTag;
                } else if (c == '>') {
                    m_currentToken.attributes.push_back({m_currentAttrName, ""});
                    advance();
                    emitToken(m_currentToken);
                    m_state = State::Data;
                } else {
                    // New attribute without value
                    m_currentToken.attributes.push_back({m_currentAttrName, ""});
                    m_currentAttrName.clear();
                    m_currentAttrVal.clear();
                    m_currentAttrName.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(advance()))));
                    m_state = State::AttributeName;
                }
                break;
            }

            case State::BeforeAttributeValue: {
                char c = peek();
                if (isWhitespace(c)) {
                    advance();
                } else if (c == '"') {
                    advance();
                    m_state = State::AttributeValueDoubleQuoted;
                } else if (c == '\'') {
                    advance();
                    m_state = State::AttributeValueSingleQuoted;
                } else if (c == '>') {
                    m_currentToken.attributes.push_back({m_currentAttrName, ""});
                    advance();
                    emitToken(m_currentToken);
                    m_state = State::Data;
                } else {
                    m_currentAttrVal.clear();
                    m_currentAttrVal.push_back(advance());
                    m_state = State::AttributeValueUnquoted;
                }
                break;
            }

            case State::AttributeValueDoubleQuoted: {
                char c = peek();
                if (c == '"') {
                    advance();
                    m_currentToken.attributes.push_back({m_currentAttrName, m_currentAttrVal});
                    m_state = State::AfterAttributeValueQuoted;
                } else if (c == '&') {
                    m_currentAttrVal += consumeCharacterReference();
                } else {
                    m_currentAttrVal.push_back(advance());
                }
                break;
            }

            case State::AttributeValueSingleQuoted: {
                char c = peek();
                if (c == '\'') {
                    advance();
                    m_currentToken.attributes.push_back({m_currentAttrName, m_currentAttrVal});
                    m_state = State::AfterAttributeValueQuoted;
                } else if (c == '&') {
                    m_currentAttrVal += consumeCharacterReference();
                } else {
                    m_currentAttrVal.push_back(advance());
                }
                break;
            }

            case State::AttributeValueUnquoted: {
                char c = peek();
                if (isWhitespace(c)) {
                    advance();
                    m_currentToken.attributes.push_back({m_currentAttrName, m_currentAttrVal});
                    m_state = State::BeforeAttributeName;
                } else if (c == '>') {
                    advance();
                    m_currentToken.attributes.push_back({m_currentAttrName, m_currentAttrVal});
                    emitToken(m_currentToken);
                    m_state = State::Data;
                } else if (c == '&') {
                    m_currentAttrVal += consumeCharacterReference();
                } else {
                    m_currentAttrVal.push_back(advance());
                }
                break;
            }

            case State::AfterAttributeValueQuoted: {
                char c = peek();
                if (isWhitespace(c)) {
                    advance();
                    m_state = State::BeforeAttributeName;
                } else if (c == '/') {
                    advance();
                    m_state = State::SelfClosingStartTag;
                } else if (c == '>') {
                    advance();
                    emitToken(m_currentToken);
                    m_state = State::Data;
                } else {
                    m_state = State::BeforeAttributeName;
                }
                break;
            }

            case State::SelfClosingStartTag: {
                char c = peek();
                if (c == '>') {
                    advance();
                    m_currentToken.selfClosing = true;
                    emitToken(m_currentToken);
                    m_state = State::Data;
                } else {
                    m_state = State::BeforeAttributeName;
                }
                break;
            }

            case State::MarkupDeclarationOpen: {
                if (match("--")) {
                    m_currentToken = HtmlToken{};
                    m_currentToken.type = TokenType::Comment;
                    m_currentToken.line = m_line;
                    m_currentToken.column = m_column;
                    m_state = State::Comment;
                } else if (match("DOCTYPE")) {
                    m_currentToken = HtmlToken{};
                    m_currentToken.type = TokenType::DOCTYPE;
                    m_currentToken.line = m_line;
                    m_currentToken.column = m_column;
                    m_state = State::BeforeDoctypeName;
                } else {
                    // Bogus comment
                    m_currentToken = HtmlToken{};
                    m_currentToken.type = TokenType::Comment;
                    m_state = State::BogusComment;
                }
                break;
            }

            case State::Comment: {
                char c = peek();
                if (c == '-') {
                    advance();
                    m_state = State::CommentEndDash;
                } else {
                    m_currentToken.data.push_back(advance());
                }
                break;
            }

            case State::CommentEndDash: {
                char c = peek();
                if (c == '-') {
                    advance();
                    m_state = State::CommentEnd;
                } else {
                    m_currentToken.data.push_back('-');
                    m_currentToken.data.push_back(advance());
                    m_state = State::Comment;
                }
                break;
            }

            case State::CommentEnd: {
                char c = peek();
                if (c == '>') {
                    advance();
                    emitToken(m_currentToken);
                    m_state = State::Data;
                } else if (c == '-') {
                    m_currentToken.data.push_back('-');
                    advance();
                } else {
                    m_currentToken.data += "--";
                    m_currentToken.data.push_back(advance());
                    m_state = State::Comment;
                }
                break;
            }

            case State::BogusComment: {
                char c = peek();
                if (c == '>') {
                    advance();
                    emitToken(m_currentToken);
                    m_state = State::Data;
                } else {
                    m_currentToken.data.push_back(advance());
                }
                break;
            }

            case State::BeforeDoctypeName: {
                char c = peek();
                if (isWhitespace(c)) {
                    advance();
                } else if (c == '>') {
                    advance();
                    m_currentToken.forceQuirks = true;
                    emitToken(m_currentToken);
                    m_state = State::Data;
                } else {
                    m_currentToken.data.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(advance()))));
                    m_state = State::DoctypeName;
                }
                break;
            }

            case State::DoctypeName: {
                char c = peek();
                if (isWhitespace(c)) {
                    advance();
                    m_state = State::AfterDoctypeName;
                } else if (c == '>') {
                    advance();
                    emitToken(m_currentToken);
                    m_state = State::Data;
                } else {
                    m_currentToken.data.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(advance()))));
                }
                break;
            }

            case State::AfterDoctypeName: {
                char c = peek();
                if (isWhitespace(c)) {
                    advance();
                } else if (c == '>') {
                    advance();
                    emitToken(m_currentToken);
                    m_state = State::Data;
                } else {
                    advance(); // Consume remaining tokens (public/system ids) until '>'
                }
                break;
            }

            default:
                m_state = State::Data;
                break;
        }
    }

    HtmlToken front = std::move(m_tokenQueue.front());
    m_tokenQueue.erase(m_tokenQueue.begin());
    return front;
}

std::vector<HtmlToken> HtmlTokenizer::tokenizeAll() {
    std::vector<HtmlToken> tokens;
    while (true) {
        auto token = nextToken();
        if (token.type == TokenType::EndOfFile) {
            tokens.push_back(std::move(token));
            break;
        }
        tokens.push_back(std::move(token));
    }
    return tokens;
}

} // namespace browser::html
