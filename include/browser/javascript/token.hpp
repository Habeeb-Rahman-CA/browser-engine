#pragma once

#include <string>
#include <string_view>

namespace browser::javascript {

enum class TokenType {
    EndOfFile,
    Number,
    String,
    True,
    False,
    Null,
    Undefined,
    Identifier,

    // Keywords
    Var,
    Let,
    Const,
    Function,
    Return,
    If,
    Else,
    While,
    For,

    // Operators
    Plus,          // +
    Minus,         // -
    Star,          // *
    Slash,         // /
    Percent,       // %
    Equal,         // =
    EqualEqual,    // ==
    BangEqual,     // !=
    Less,          // <
    LessEqual,     // <=
    Greater,       // >
    GreaterEqual,  // >=
    Bang,          // !
    AmpAmp,        // &&
    PipePipe,      // ||
    PlusPlus,      // ++
    MinusMinus,    // --

    // Delimiters
    LeftParen,     // (
    RightParen,    // )
    LeftBrace,     // {
    RightBrace,    // }
    LeftBracket,   // [
    RightBracket,  // ]
    Semicolon,     // ;
    Comma,         // ,
    Dot            // .
};

struct Token {
    TokenType type{TokenType::EndOfFile};
    std::string text;
    double numberValue{0.0};
    size_t line{1};
    size_t column{1};

    [[nodiscard]] std::string_view typeName() const noexcept;
};

} // namespace browser::javascript

