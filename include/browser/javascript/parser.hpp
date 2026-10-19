#pragma once

#include "browser/javascript/token.hpp"
#include "browser/javascript/ast.hpp"
#include <vector>
#include <memory>
#include <string_view>

namespace browser::javascript {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    [[nodiscard]] std::vector<std::unique_ptr<Statement>> parse();

private:
    // Statement parsing
    [[nodiscard]] std::unique_ptr<Statement> declaration();
    [[nodiscard]] std::unique_ptr<Statement> varDeclaration();
    [[nodiscard]] std::unique_ptr<Statement> functionDeclaration();
    [[nodiscard]] std::unique_ptr<Statement> statement();
    [[nodiscard]] std::unique_ptr<Statement> ifStatement();
    [[nodiscard]] std::unique_ptr<Statement> whileStatement();
    [[nodiscard]] std::unique_ptr<Statement> forStatement();
    [[nodiscard]] std::unique_ptr<Statement> returnStatement();
    [[nodiscard]] std::unique_ptr<Statement> blockStatement();
    [[nodiscard]] std::unique_ptr<Statement> expressionStatement();

    // Expression parsing
    [[nodiscard]] std::unique_ptr<Expression> expression();
    [[nodiscard]] std::unique_ptr<Expression> assignment();
    [[nodiscard]] std::unique_ptr<Expression> logicalOr();
    [[nodiscard]] std::unique_ptr<Expression> logicalAnd();
    [[nodiscard]] std::unique_ptr<Expression> equality();
    [[nodiscard]] std::unique_ptr<Expression> comparison();
    [[nodiscard]] std::unique_ptr<Expression> term();
    [[nodiscard]] std::unique_ptr<Expression> factor();
    [[nodiscard]] std::unique_ptr<Expression> unary();
    [[nodiscard]] std::unique_ptr<Expression> callOrMember();
    [[nodiscard]] std::unique_ptr<Expression> primary();

    // Token helpers
    [[nodiscard]] bool isAtEnd() const noexcept;
    const Token& peek() const noexcept;
    const Token& previous() const noexcept;
    const Token& advance();
    bool check(TokenType type) const noexcept;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    const Token& consume(TokenType type, std::string_view message);

    void synchronize();

    std::vector<Token> m_tokens;
    size_t m_current{0};
};

} // namespace browser::javascript

