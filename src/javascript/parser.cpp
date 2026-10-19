#include "browser/javascript/parser.hpp"
#include <stdexcept>
#include <iostream>

namespace browser::javascript {

Parser::Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens)) {}

std::vector<std::unique_ptr<Statement>> Parser::parse() {
    std::vector<std::unique_ptr<Statement>> statements;
    while (!isAtEnd()) {
        if (peek().type == TokenType::Semicolon) {
            advance();
            continue;
        }
        try {
            auto stmt = declaration();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
        } catch (const std::runtime_error& err) {
            synchronize();
        }
    }
    return statements;
}

std::unique_ptr<Statement> Parser::declaration() {
    if (match({TokenType::Var, TokenType::Let, TokenType::Const})) {
        return varDeclaration();
    }
    if (match(TokenType::Function)) {
        return functionDeclaration();
    }
    return statement();
}

std::unique_ptr<Statement> Parser::varDeclaration() {
    const auto& nameToken = consume(TokenType::Identifier, "Expected variable name after declaration keyword");
    std::string name = nameToken.text;

    std::unique_ptr<Expression> initializer = nullptr;
    if (match(TokenType::Equal)) {
        initializer = expression();
    }

    if (match(TokenType::Semicolon)) {
        // consumed optional semicolon
    }
    return std::make_unique<VarDeclStmt>(std::move(name), std::move(initializer));
}

std::unique_ptr<Statement> Parser::functionDeclaration() {
    const auto& nameToken = consume(TokenType::Identifier, "Expected function name");
    std::string name = nameToken.text;

    consume(TokenType::LeftParen, "Expected '(' after function name");
    std::vector<std::string> params;
    if (!check(TokenType::RightParen)) {
        do {
            const auto& param = consume(TokenType::Identifier, "Expected parameter name");
            params.push_back(param.text);
        } while (match(TokenType::Comma));
    }
    consume(TokenType::RightParen, "Expected ')' after parameters");

    consume(TokenType::LeftBrace, "Expected '{' before function body");
    auto body = blockStatement();

    return std::make_unique<FunctionStmt>(std::move(name), std::move(params), std::move(body));
}

std::unique_ptr<Statement> Parser::statement() {
    if (match(TokenType::If)) return ifStatement();
    if (match(TokenType::While)) return whileStatement();
    if (match(TokenType::For)) return forStatement();
    if (match(TokenType::Return)) return returnStatement();
    if (match(TokenType::LeftBrace)) return blockStatement();

    return expressionStatement();
}

std::unique_ptr<Statement> Parser::ifStatement() {
    consume(TokenType::LeftParen, "Expected '(' after 'if'");
    auto condition = expression();
    consume(TokenType::RightParen, "Expected ')' after if condition");

    auto thenBranch = statement();
    std::unique_ptr<Statement> elseBranch = nullptr;
    if (match(TokenType::Else)) {
        elseBranch = statement();
    }

    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<Statement> Parser::whileStatement() {
    consume(TokenType::LeftParen, "Expected '(' after 'while'");
    auto condition = expression();
    consume(TokenType::RightParen, "Expected ')' after while condition");

    auto body = statement();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Statement> Parser::forStatement() {
    consume(TokenType::LeftParen, "Expected '(' after 'for'");

    std::unique_ptr<Statement> initializer = nullptr;
    if (match(TokenType::Semicolon)) {
        initializer = nullptr;
    } else if (match({TokenType::Var, TokenType::Let, TokenType::Const})) {
        initializer = varDeclaration();
    } else {
        initializer = expressionStatement();
    }

    std::unique_ptr<Expression> condition = nullptr;
    if (!check(TokenType::Semicolon)) {
        condition = expression();
    }
    consume(TokenType::Semicolon, "Expected ';' after loop condition");

    std::unique_ptr<Expression> update = nullptr;
    if (!check(TokenType::RightParen)) {
        update = expression();
    }
    consume(TokenType::RightParen, "Expected ')' after for clauses");

    auto body = statement();
    return std::make_unique<ForStmt>(std::move(initializer), std::move(condition), std::move(update), std::move(body));
}

std::unique_ptr<Statement> Parser::returnStatement() {
    std::unique_ptr<Expression> value = nullptr;
    if (!check(TokenType::Semicolon) && !check(TokenType::RightBrace) && !isAtEnd()) {
        value = expression();
    }
    if (match(TokenType::Semicolon)) {}
    return std::make_unique<ReturnStmt>(std::move(value));
}

std::unique_ptr<Statement> Parser::blockStatement() {
    std::vector<std::unique_ptr<Statement>> statements;
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        statements.push_back(declaration());
    }
    consume(TokenType::RightBrace, "Expected '}' after block");
    return std::make_unique<BlockStmt>(std::move(statements));
}

std::unique_ptr<Statement> Parser::expressionStatement() {
    auto expr = expression();
    if (match(TokenType::Semicolon)) {}
    return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<Expression> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expression> Parser::assignment() {
    auto expr = logicalOr();

    if (match(TokenType::Equal)) {
        auto value = assignment();

        if (auto* id = dynamic_cast<IdentifierExpr*>(expr.get())) {
            return std::make_unique<AssignExpr>(id->name, std::move(value));
        }
        if (auto* mem = dynamic_cast<MemberExpr*>(expr.get())) {
            return std::make_unique<MemberAssignExpr>(std::move(mem->object), mem->property, std::move(value));
        }

        throw std::runtime_error("Invalid assignment target");
    }

    return expr;
}

std::unique_ptr<Expression> Parser::logicalOr() {
    auto expr = logicalAnd();

    while (match(TokenType::PipePipe)) {
        std::string op = previous().text;
        auto right = logicalAnd();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

std::unique_ptr<Expression> Parser::logicalAnd() {
    auto expr = equality();

    while (match(TokenType::AmpAmp)) {
        std::string op = previous().text;
        auto right = equality();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

std::unique_ptr<Expression> Parser::equality() {
    auto expr = comparison();

    while (match({TokenType::EqualEqual, TokenType::BangEqual})) {
        std::string op = previous().text;
        auto right = comparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

std::unique_ptr<Expression> Parser::comparison() {
    auto expr = term();

    while (match({TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual})) {
        std::string op = previous().text;
        auto right = term();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

std::unique_ptr<Expression> Parser::term() {
    auto expr = factor();

    while (match({TokenType::Plus, TokenType::Minus})) {
        std::string op = previous().text;
        auto right = factor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

std::unique_ptr<Expression> Parser::factor() {
    auto expr = unary();

    while (match({TokenType::Star, TokenType::Slash, TokenType::Percent})) {
        std::string op = previous().text;
        auto right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

std::unique_ptr<Expression> Parser::unary() {
    if (match({TokenType::Bang, TokenType::Minus, TokenType::Plus})) {
        std::string op = previous().text;
        auto right = unary();
        return std::make_unique<UnaryExpr>(std::move(op), std::move(right));
    }

    if (match(TokenType::PlusPlus)) {
        const auto& id = consume(TokenType::Identifier, "Expected identifier after '++'");
        return std::make_unique<UpdateExpr>(id.text, true, true);
    }
    if (match(TokenType::MinusMinus)) {
        const auto& id = consume(TokenType::Identifier, "Expected identifier after '--'");
        return std::make_unique<UpdateExpr>(id.text, false, true);
    }

    return callOrMember();
}

std::unique_ptr<Expression> Parser::callOrMember() {
    auto expr = primary();

    while (true) {
        if (match(TokenType::LeftParen)) {
            // Function call
            std::vector<std::unique_ptr<Expression>> args;
            if (!check(TokenType::RightParen)) {
                do {
                    args.push_back(expression());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RightParen, "Expected ')' after function arguments");
            expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
        } else if (match(TokenType::Dot)) {
            // Member access: expr.prop
            const auto& name = consume(TokenType::Identifier, "Expected property name after '.'");
            expr = std::make_unique<MemberExpr>(std::move(expr), name.text);
        } else if (match(TokenType::LeftBracket)) {
            // Index access: expr[idx]
            auto idx = expression();
            consume(TokenType::RightBracket, "Expected ']' after index expression");
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(idx));
        } else if (match(TokenType::PlusPlus)) {
            if (auto* id = dynamic_cast<IdentifierExpr*>(expr.get())) {
                expr = std::make_unique<UpdateExpr>(id->name, true, false);
            } else {
                throw std::runtime_error("Invalid postfix '++' target");
            }
        } else if (match(TokenType::MinusMinus)) {
            if (auto* id = dynamic_cast<IdentifierExpr*>(expr.get())) {
                expr = std::make_unique<UpdateExpr>(id->name, false, false);
            } else {
                throw std::runtime_error("Invalid postfix '--' target");
            }
        } else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<Expression> Parser::primary() {
    if (match(TokenType::False)) return std::make_unique<LiteralExpr>(Value(false));
    if (match(TokenType::True)) return std::make_unique<LiteralExpr>(Value(true));
    if (match(TokenType::Null)) return std::make_unique<LiteralExpr>(Value(nullptr));
    if (match(TokenType::Undefined)) return std::make_unique<LiteralExpr>(Value());

    if (match(TokenType::Number)) {
        return std::make_unique<LiteralExpr>(Value(previous().numberValue));
    }
    if (match(TokenType::String)) {
        return std::make_unique<LiteralExpr>(Value(previous().text));
    }

    if (match(TokenType::Identifier)) {
        return std::make_unique<IdentifierExpr>(previous().text);
    }

    if (match(TokenType::LeftParen)) {
        auto expr = expression();
        consume(TokenType::RightParen, "Expected ')' after parenthesized expression");
        return expr;
    }

    if (match(TokenType::LeftBracket)) {
        std::vector<std::unique_ptr<Expression>> elements;
        if (!check(TokenType::RightBracket)) {
            do {
                elements.push_back(expression());
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RightBracket, "Expected ']' after array elements");
        return std::make_unique<ArrayLiteralExpr>(std::move(elements));
    }

    if (match(TokenType::Function)) {
        std::string name = "anonymous";
        if (check(TokenType::Identifier)) {
            name = advance().text;
        }
        consume(TokenType::LeftParen, "Expected '(' after function");
        std::vector<std::string> params;
        if (!check(TokenType::RightParen)) {
            do {
                const auto& param = consume(TokenType::Identifier, "Expected parameter name");
                params.push_back(param.text);
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RightParen, "Expected ')' after parameters");
        consume(TokenType::LeftBrace, "Expected '{' before function body");
        auto body = blockStatement();
        return std::make_unique<FunctionExpr>(std::move(name), std::move(params), std::move(body));
    }

    throw std::runtime_error("Unexpected token in expression: " + peek().text);
}

bool Parser::isAtEnd() const noexcept {
    return peek().type == TokenType::EndOfFile;
}

const Token& Parser::peek() const noexcept {
    return m_tokens[m_current];
}

const Token& Parser::previous() const noexcept {
    return m_tokens[m_current - 1];
}

const Token& Parser::advance() {
    if (!isAtEnd()) m_current++;
    return previous();
}

bool Parser::check(TokenType type) const noexcept {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

const Token& Parser::consume(TokenType type, std::string_view message) {
    if (check(type)) return advance();
    throw std::runtime_error(std::string(message) + " (found: " + peek().text + ")");
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (previous().type == TokenType::Semicolon) return;
        switch (peek().type) {
            case TokenType::Function:
            case TokenType::Var:
            case TokenType::Let:
            case TokenType::Const:
            case TokenType::For:
            case TokenType::If:
            case TokenType::While:
            case TokenType::Return:
                return;
            default:
                advance();
                break;
        }
    }
}

} // namespace browser::javascript
