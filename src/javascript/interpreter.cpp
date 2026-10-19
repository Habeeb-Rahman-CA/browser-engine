#include "browser/javascript/interpreter.hpp"
#include "browser/javascript/lexer.hpp"
#include "browser/javascript/parser.hpp"
#include "browser/core/diagnostics.hpp"

#include <cmath>
#include <iostream>

namespace browser::javascript {

namespace {

struct ReturnSignal {
    Value value;
};

} // namespace

Interpreter::Interpreter(InterpreterLimits limits)
    : m_limits(limits), m_global(std::make_shared<Environment>()) {
    // Provide default console object: console.log(...)
    auto consoleObj = Value::makeObject();
    consoleObj.setProperty("log", Value([](const std::vector<Value>& args) -> Value {
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) std::cout << " ";
            std::cout << args[i].toString();
        }
        std::cout << std::endl;
        return Value();
    }, "log"));

    m_global->define("console", std::move(consoleObj));
}

void Interpreter::setGlobal(const std::string& name, Value val) {
    m_global->define(name, std::move(val));
}

Value Interpreter::getGlobal(const std::string& name) const {
    auto res = m_global->get(name);
    return res ? *res : Value();
}

void Interpreter::tickStep() {
    m_stepCount++;
    if (m_stepCount > m_limits.maxSteps) {
        throw RuntimeError("Execution limit exceeded: max steps reached (" +
                           std::to_string(m_limits.maxSteps) + ")");
    }
}

Value Interpreter::evaluate(std::string_view source) {
    core::ScopedTimer timer(core::Subsystem::JavaScript);
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        auto statements = parser.parse();
        return execute(statements);
    } catch (const std::exception& e) {
        LOG_ERROR("JavaScript", "Script evaluation error: " + std::string(e.what()));
        return Value();
    }
}

Value Interpreter::execute(const std::vector<std::unique_ptr<Statement>>& statements) {
    Value lastValue;
    try {
        for (const auto& stmt : statements) {
            lastValue = executeStatement(stmt.get(), m_global);
        }
    } catch (const ReturnSignal& sig) {
        return sig.value;
    }
    return lastValue;
}

Value Interpreter::executeStatement(const Statement* stmt, std::shared_ptr<Environment> env) {
    if (!stmt) return Value();
    tickStep();

    if (const auto* exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
        return evaluateExpression(exprStmt->expression.get(), env);
    } else if (const auto* varStmt = dynamic_cast<const VarDeclStmt*>(stmt)) {
        Value val;
        if (varStmt->initializer) {
            val = evaluateExpression(varStmt->initializer.get(), env);
        }
        env->define(varStmt->name, val);
        return val;
    } else if (const auto* funcStmt = dynamic_cast<const FunctionStmt*>(stmt)) {
        auto fnData = std::make_shared<FunctionData>();
        fnData->name = funcStmt->name;
        fnData->params = funcStmt->params;
        fnData->body = funcStmt->body;
        fnData->closure = env; // Capture current lexical scope
        fnData->isNative = false;
        Value fnVal(fnData);
        env->define(funcStmt->name, fnVal);
        return fnVal;
    } else if (const auto* returnStmt = dynamic_cast<const ReturnStmt*>(stmt)) {
        Value val;
        if (returnStmt->value) {
            val = evaluateExpression(returnStmt->value.get(), env);
        }
        throw ReturnSignal{std::move(val)};
    } else if (const auto* ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        Value cond = evaluateExpression(ifStmt->condition.get(), env);
        if (cond.toBoolean()) {
            return executeStatement(ifStmt->thenBranch.get(), env);
        } else if (ifStmt->elseBranch) {
            return executeStatement(ifStmt->elseBranch.get(), env);
        }
    } else if (const auto* whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        Value last;
        while (evaluateExpression(whileStmt->condition.get(), env).toBoolean()) {
            tickStep();
            last = executeStatement(whileStmt->body.get(), env);
        }
        return last;
    } else if (const auto* forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        auto forEnv = std::make_shared<Environment>(env);
        if (forStmt->initializer) {
            executeStatement(forStmt->initializer.get(), forEnv);
        }
        Value last;
        while (!forStmt->condition || evaluateExpression(forStmt->condition.get(), forEnv).toBoolean()) {
            tickStep();
            last = executeStatement(forStmt->body.get(), forEnv);
            if (forStmt->update) {
                evaluateExpression(forStmt->update.get(), forEnv);
            }
        }
        return last;
    } else if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(stmt)) {
        auto blockEnv = std::make_shared<Environment>(env);
        Value last;
        for (const auto& s : blockStmt->statements) {
            last = executeStatement(s.get(), blockEnv);
        }
        return last;
    }
    return Value();
}

Value Interpreter::evaluateExpression(const Expression* expr, std::shared_ptr<Environment> env) {
    if (!expr) return Value();
    tickStep();

    if (const auto* lit = dynamic_cast<const LiteralExpr*>(expr)) {
        return lit->value;
    }

    if (const auto* id = dynamic_cast<const IdentifierExpr*>(expr)) {
        auto val = env->get(id->name);
        if (val) return *val;
        return Value(); // undefined
    }

    if (const auto* assign = dynamic_cast<const AssignExpr*>(expr)) {
        Value val = evaluateExpression(assign->value.get(), env);
        env->set(assign->name, val);
        return val;
    }

    if (const auto* memAssign = dynamic_cast<const MemberAssignExpr*>(expr)) {
        Value obj = evaluateExpression(memAssign->object.get(), env);
        Value val = evaluateExpression(memAssign->value.get(), env);
        obj.setProperty(memAssign->property, val);
        return val;
    }

    if (const auto* unary = dynamic_cast<const UnaryExpr*>(expr)) {
        Value right = evaluateExpression(unary->right.get(), env);
        if (unary->op == "!") {
            return Value(!right.toBoolean());
        }
        if (unary->op == "-") {
            return Value(-right.toNumber());
        }
        if (unary->op == "+") {
            return Value(right.toNumber());
        }
        return Value();
    }

    if (const auto* upd = dynamic_cast<const UpdateExpr*>(expr)) {
        auto cur = env->get(upd->name);
        double oldNum = cur ? cur->toNumber() : 0.0;
        double newNum = upd->isIncrement ? (oldNum + 1.0) : (oldNum - 1.0);
        env->set(upd->name, Value(newNum));
        return upd->isPrefix ? Value(newNum) : Value(oldNum);
    }

    if (const auto* binary = dynamic_cast<const BinaryExpr*>(expr)) {
        // Short-circuiting for logical operators
        if (binary->op == "&&") {
            Value left = evaluateExpression(binary->left.get(), env);
            if (!left.toBoolean()) return left;
            return evaluateExpression(binary->right.get(), env);
        }
        if (binary->op == "||") {
            Value left = evaluateExpression(binary->left.get(), env);
            if (left.toBoolean()) return left;
            return evaluateExpression(binary->right.get(), env);
        }

        Value left = evaluateExpression(binary->left.get(), env);
        Value right = evaluateExpression(binary->right.get(), env);

        if (binary->op == "+") {
            if (left.isString() || right.isString()) {
                return Value(left.toString() + right.toString());
            }
            return Value(left.toNumber() + right.toNumber());
        }
        if (binary->op == "-") return Value(left.toNumber() - right.toNumber());
        if (binary->op == "*") return Value(left.toNumber() * right.toNumber());
        if (binary->op == "/") {
            double denom = right.toNumber();
            if (denom == 0.0) return Value(std::nan(""));
            return Value(left.toNumber() / denom);
        }
        if (binary->op == "%") {
            double denom = right.toNumber();
            if (denom == 0.0) return Value(std::nan(""));
            return Value(std::fmod(left.toNumber(), denom));
        }

        if (binary->op == "==") return Value(left.equals(right));
        if (binary->op == "!=") return Value(!left.equals(right));
        if (binary->op == "<") return Value(left.toNumber() < right.toNumber());
        if (binary->op == "<=") return Value(left.toNumber() <= right.toNumber());
        if (binary->op == ">") return Value(left.toNumber() > right.toNumber());
        if (binary->op == ">=") return Value(left.toNumber() >= right.toNumber());

        return Value();
    }

    if (const auto* mem = dynamic_cast<const MemberExpr*>(expr)) {
        Value obj = evaluateExpression(mem->object.get(), env);
        return obj.getProperty(mem->property);
    }

    if (const auto* idx = dynamic_cast<const IndexExpr*>(expr)) {
        Value obj = evaluateExpression(idx->object.get(), env);
        Value key = evaluateExpression(idx->index.get(), env);
        return obj.getProperty(key.toString());
    }

    if (const auto* arr = dynamic_cast<const ArrayLiteralExpr*>(expr)) {
        std::vector<Value> items;
        for (const auto& item : arr->elements) {
            items.push_back(evaluateExpression(item.get(), env));
        }
        return Value::makeArray(std::move(items));
    }

    if (const auto* call = dynamic_cast<const CallExpr*>(expr)) {
        Value callee = evaluateExpression(call->callee.get(), env);
        std::vector<Value> args;
        for (const auto& arg : call->args) {
            args.push_back(evaluateExpression(arg.get(), env));
        }
        return callFunction(callee, args);
    }

    if (const auto* fnExpr = dynamic_cast<const FunctionExpr*>(expr)) {
        auto fnData = std::make_shared<FunctionData>();
        fnData->name = fnExpr->name;
        fnData->params = fnExpr->params;
        fnData->body = fnExpr->body;
        fnData->closure = env;
        fnData->isNative = false;
        return Value(fnData);
    }

    return Value();
}

Value Interpreter::callFunction(const Value& callee, const std::vector<Value>& args) {
    if (!callee.isFunction()) {
        return Value();
    }

    const auto& fnData = callee.functionData();
    if (!fnData) return Value();

    if (fnData->isNative && fnData->native) {
        return fnData->native(args);
    }

    // User-defined function
    if (m_callDepth >= m_limits.maxRecursionDepth) {
        throw RuntimeError("Maximum call stack size exceeded (" +
                           std::to_string(m_limits.maxRecursionDepth) + ")");
    }

    m_callDepth++;
    auto fnEnv = std::make_shared<Environment>(fnData->closure ? fnData->closure : m_global);

    // Bind parameters
    for (size_t i = 0; i < fnData->params.size(); ++i) {
        Value argVal = (i < args.size()) ? args[i] : Value();
        fnEnv->define(fnData->params[i], std::move(argVal));
    }

    Value retVal;
    try {
        executeStatement(fnData->body.get(), fnEnv);
    } catch (const ReturnSignal& sig) {
        retVal = sig.value;
    }
    m_callDepth--;

    return retVal;
}

} // namespace browser::javascript
