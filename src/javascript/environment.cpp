#include "browser/javascript/environment.hpp"

namespace browser::javascript {

void Environment::define(const std::string& name, Value val) {
    m_values[name] = std::move(val);
}

bool Environment::set(const std::string& name, Value val) {
    auto it = m_values.find(name);
    if (it != m_values.end()) {
        it->second = std::move(val);
        return true;
    }
    if (m_parent) {
        return m_parent->set(name, std::move(val));
    }
    // If variable wasn't declared with let/var/const, JS assigns to global scope:
    m_values[name] = std::move(val);
    return true;
}

std::optional<Value> Environment::get(const std::string& name) const {
    auto it = m_values.find(name);
    if (it != m_values.end()) {
        return it->second;
    }
    if (m_parent) {
        return m_parent->get(name);
    }
    return std::nullopt;
}

bool Environment::has(const std::string& name) const {
    if (m_values.contains(name)) return true;
    if (m_parent) return m_parent->has(name);
    return false;
}

} // namespace browser::javascript

