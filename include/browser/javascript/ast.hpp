#pragma once

#include "browser/javascript/value.hpp"
#include <string>
#include <vector>
#include <memory>

namespace browser::javascript {

// Forward declare visitor or interpreter
class ASTVisitor;

class ASTNode {
public:
    virtual ~ASTNode() = default;
};

class Expression : public ASTNode {};
class Statement : public ASTNode {};

// --- Expressions ---

class LiteralExpr : public Expression {
public:
    explicit LiteralExpr(Value val) : value(std::move(val)) {}
    Value value;
};

class IdentifierExpr : public Expression {
public:
    explicit IdentifierExpr(std::string n) : name(std::move(n)) {}
    std::string name;
};

class UnaryExpr : public Expression {
public:
    UnaryExpr(std::string operatorName, std::unique_ptr<Expression> r)
        : op(std::move(operatorName)), right(std::move(r)) {}
    std::string op;
    std::unique_ptr<Expression> right;
};

class BinaryExpr : public Expression {
public:
    BinaryExpr(std::unique_ptr<Expression> l, std::string operatorName, std::unique_ptr<Expression> r)
        : left(std::move(l)), op(std::move(operatorName)), right(std::move(r)) {}
    std::unique_ptr<Expression> left;
    std::string op;
    std::unique_ptr<Expression> right;
};

class AssignExpr : public Expression {
public:
    AssignExpr(std::string n, std::unique_ptr<Expression> val)
        : name(std::move(n)), value(std::move(val)) {}
    std::string name;
    std::unique_ptr<Expression> value;
};

class MemberAssignExpr : public Expression {
public:
    MemberAssignExpr(std::unique_ptr<Expression> obj, std::string prop, std::unique_ptr<Expression> val)
        : object(std::move(obj)), property(std::move(prop)), value(std::move(val)) {}
    std::unique_ptr<Expression> object;
    std::string property;
    std::unique_ptr<Expression> value;
};

class CallExpr : public Expression {
public:
    CallExpr(std::unique_ptr<Expression> c, std::vector<std::unique_ptr<Expression>> a)
        : callee(std::move(c)), args(std::move(a)) {}
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> args;
};

class MemberExpr : public Expression {
public:
    MemberExpr(std::unique_ptr<Expression> obj, std::string prop)
        : object(std::move(obj)), property(std::move(prop)) {}
    std::unique_ptr<Expression> object;
    std::string property;
};

class IndexExpr : public Expression {
public:
    IndexExpr(std::unique_ptr<Expression> obj, std::unique_ptr<Expression> idx)
        : object(std::move(obj)), index(std::move(idx)) {}
    std::unique_ptr<Expression> object;
    std::unique_ptr<Expression> index;
};

class ArrayLiteralExpr : public Expression {
public:
    explicit ArrayLiteralExpr(std::vector<std::unique_ptr<Expression>> el)
        : elements(std::move(el)) {}
    std::vector<std::unique_ptr<Expression>> elements;
};

class UpdateExpr : public Expression {
public:
    UpdateExpr(std::string n, bool inc, bool pre)
        : name(std::move(n)), isIncrement(inc), isPrefix(pre) {}
    std::string name;
    bool isIncrement{true};
    bool isPrefix{false};
};

class FunctionExpr : public Expression {
public:
    FunctionExpr(std::string n, std::vector<std::string> p, std::shared_ptr<Statement> b)
        : name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<Statement> body;
};

// --- Statements ---

class ExprStmt : public Statement {
public:
    explicit ExprStmt(std::unique_ptr<Expression> expr) : expression(std::move(expr)) {}
    std::unique_ptr<Expression> expression;
};

class VarDeclStmt : public Statement {
public:
    VarDeclStmt(std::string n, std::unique_ptr<Expression> init)
        : name(std::move(n)), initializer(std::move(init)) {}
    std::string name;
    std::unique_ptr<Expression> initializer;
};

class BlockStmt : public Statement {
public:
    explicit BlockStmt(std::vector<std::unique_ptr<Statement>> stmts)
        : statements(std::move(stmts)) {}
    std::vector<std::unique_ptr<Statement>> statements;
};

class IfStmt : public Statement {
public:
    IfStmt(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> thn, std::unique_ptr<Statement> els = nullptr)
        : condition(std::move(cond)), thenBranch(std::move(thn)), elseBranch(std::move(els)) {}
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch;
};

class WhileStmt : public Statement {
public:
    WhileStmt(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> b)
        : condition(std::move(cond)), body(std::move(b)) {}
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
};

class ForStmt : public Statement {
public:
    ForStmt(std::unique_ptr<Statement> init, std::unique_ptr<Expression> cond,
            std::unique_ptr<Expression> upd, std::unique_ptr<Statement> b)
        : initializer(std::move(init)), condition(std::move(cond)),
          update(std::move(upd)), body(std::move(b)) {}
    std::unique_ptr<Statement> initializer;
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> update;
    std::unique_ptr<Statement> body;
};

class FunctionStmt : public Statement {
public:
    FunctionStmt(std::string n, std::vector<std::string> p, std::shared_ptr<Statement> b)
        : name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<Statement> body;
};

class ReturnStmt : public Statement {
public:
    explicit ReturnStmt(std::unique_ptr<Expression> val = nullptr)
        : value(std::move(val)) {}
    std::unique_ptr<Expression> value;
};

} // namespace browser::javascript
