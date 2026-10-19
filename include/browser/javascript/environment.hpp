#pragma once

#include "browser/javascript/value.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <optional>

namespace browser::javascript {

class Environment : public std::enable_shared_from_this<Environment> {
public:
    Environment() = default;
    explicit Environment(std::shared_ptr<Environment> parent)
        : m_parent(std::move(parent)) {}

    void define(const std::string& name, Value val);
    bool set(const std::string& name, Value val);
    [[nodiscard]] std::optional<Value> get(const std::string& name) const;
    [[nodiscard]] bool has(const std::string& name) const;

    [[nodiscard]] std::shared_ptr<Environment> parent() const noexcept { return m_parent; }

private:
    std::shared_ptr<Environment> m_parent{nullptr};
    std::unordered_map<std::string, Value> m_values;
};

} // namespace browser::javascript

