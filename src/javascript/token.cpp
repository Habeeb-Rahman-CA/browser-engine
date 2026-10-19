#include "browser/javascript/token.hpp"

namespace browser::javascript {

std::string_view Token::typeName() const noexcept {
    switch (type) {
        case TokenType::EndOfFile: return "EndOfFile";
        case TokenType::Number: return "Number";
        case TokenType::String: return "String";
        case TokenType::True: return "true";
        case TokenType::False: return "false";
        case TokenType::Null: return "null";
        case TokenType::Undefined: return "undefined";
        case TokenType::Identifier: return "Identifier";
        case TokenType::Var: return "var";
        case TokenType::Let: return "let";
        case TokenType::Const: return "const";
        case TokenType::Function: return "function";
        case TokenType::Return: return "return";
        case TokenType::If: return "if";
        case TokenType::Else: return "else";
        case TokenType::While: return "while";
        case TokenType::For: return "for";
        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
        case TokenType::Star: return "*";
        case TokenType::Slash: return "/";
        case TokenType::Percent: return "%";
        case TokenType::Equal: return "=";
        case TokenType::EqualEqual: return "==";
        case TokenType::BangEqual: return "!=";
        case TokenType::Less: return "<";
        case TokenType::LessEqual: return "<=";
        case TokenType::Greater: return ">";
        case TokenType::GreaterEqual: return ">=";
        case TokenType::Bang: return "!";
        case TokenType::AmpAmp: return "&&";
        case TokenType::PipePipe: return "||";
        case TokenType::PlusPlus: return "++";
        case TokenType::MinusMinus: return "--";
        case TokenType::LeftParen: return "(";
        case TokenType::RightParen: return ")";
        case TokenType::LeftBrace: return "{";
        case TokenType::RightBrace: return "}";
        case TokenType::LeftBracket: return "[";
        case TokenType::RightBracket: return "]";
        case TokenType::Semicolon: return ";";
        case TokenType::Comma: return ",";
        case TokenType::Dot: return ".";
    }
    return "Unknown";
}

} // namespace browser::javascript

