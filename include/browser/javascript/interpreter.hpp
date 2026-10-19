#pragma once

#include "browser/javascript/ast.hpp"
#include "browser/javascript/environment.hpp"
#include "browser/javascript/value.hpp"

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

namespace browser::javascript {

struct InterpreterLimits {
    size_t maxSteps{100000};
    size_t maxRecursionDepth{256};
};

class RuntimeError : public std::runtime_error {
public:
    explicit RuntimeError(const std::string& msg) : std::runtime_error(msg) {}
};

class Interpreter {
public:
    explicit Interpreter(InterpreterLimits limits = {});

    // Execute arbitrary JavaScript source string
    Value evaluate(std::string_view source);

    // Execute pre-parsed statements in the current environment
    Value execute(const std::vector<std::unique_ptr<Statement>>& statements);

    // Call a function value with arguments
    Value callFunction(const Value& callee, const std::vector<Value>& args);

    // Environment & global access
    [[nodiscard]] std::shared_ptr<Environment> globalEnvironment() const noexcept { return m_global; }
    void setGlobal(const std::string& name, Value val);
    [[nodiscard]] Value getGlobal(const std::string& name) const;

    // Reset step counter
    void resetStepCount() noexcept { m_stepCount = 0; }
    [[nodiscard]] size_t stepCount() const noexcept { return m_stepCount; }

    // Evaluator methods
    Value executeStatement(const Statement* stmt, std::shared_ptr<Environment> env);
    Value evaluateExpression(const Expression* expr, std::shared_ptr<Environment> env);

private:
    void tickStep();

    InterpreterLimits m_limits;
    size_t m_stepCount{0};
    size_t m_callDepth{0};
    std::shared_ptr<Environment> m_global;
};

} // namespace browser::javascript
