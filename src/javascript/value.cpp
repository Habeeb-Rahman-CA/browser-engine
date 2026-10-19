#include "browser/javascript/value.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace browser::javascript {

Value::Value() : m_type(ValueType::Undefined) {}
Value::Value(std::nullptr_t) : m_type(ValueType::Null) {}
Value::Value(bool b) : m_type(ValueType::Boolean), m_bool(b) {}
Value::Value(double d) : m_type(ValueType::Number), m_number(d) {}
Value::Value(int i) : m_type(ValueType::Number), m_number(static_cast<double>(i)) {}
Value::Value(size_t s) : m_type(ValueType::Number), m_number(static_cast<double>(s)) {}
Value::Value(const char* s) : m_type(ValueType::String), m_string(s ? s : "") {}
Value::Value(std::string s) : m_type(ValueType::String), m_string(std::move(s)) {}

Value::Value(NativeFunction fn, std::string name)
    : m_type(ValueType::Function) {
    m_function = std::make_shared<FunctionData>();
    m_function->name = std::move(name);
    m_function->native = std::move(fn);
    m_function->isNative = true;
}

Value::Value(std::shared_ptr<ObjectData> obj)
    : m_type(ValueType::Object), m_object(std::move(obj)) {}

Value::Value(std::shared_ptr<ArrayData> arr)
    : m_type(ValueType::Array), m_array(std::move(arr)) {}

Value::Value(std::shared_ptr<FunctionData> fn)
    : m_type(ValueType::Function), m_function(std::move(fn)) {}

Value Value::makeObject() {
    return Value(std::make_shared<ObjectData>());
}

Value Value::makeArray(std::vector<Value> items) {
    auto arr = std::make_shared<ArrayData>();
    arr->elements = std::move(items);
    return Value(arr);
}

bool Value::toBoolean() const noexcept {
    switch (m_type) {
        case ValueType::Undefined:
        case ValueType::Null:
            return false;
        case ValueType::Boolean:
            return m_bool;
        case ValueType::Number:
            return m_number != 0.0 && !std::isnan(m_number);
        case ValueType::String:
            return !m_string.empty();
        case ValueType::Object:
        case ValueType::Array:
        case ValueType::Function:
            return true;
    }
    return false;
}

double Value::toNumber() const noexcept {
    switch (m_type) {
        case ValueType::Undefined:
            return std::nan("");
        case ValueType::Null:
            return 0.0;
        case ValueType::Boolean:
            return m_bool ? 1.0 : 0.0;
        case ValueType::Number:
            return m_number;
        case ValueType::String:
            try {
                return std::stod(m_string);
            } catch (...) {
                return std::nan("");
            }
        case ValueType::Object:
        case ValueType::Array:
        case ValueType::Function:
            return std::nan("");
    }
    return 0.0;
}

std::string Value::toString() const {
    switch (m_type) {
        case ValueType::Undefined: return "undefined";
        case ValueType::Null: return "null";
        case ValueType::Boolean: return m_bool ? "true" : "false";
        case ValueType::Number: {
            if (std::isnan(m_number)) return "NaN";
            if (std::isinf(m_number)) return m_number > 0 ? "Infinity" : "-Infinity";
            if (m_number == static_cast<int64_t>(m_number)) {
                return std::to_string(static_cast<int64_t>(m_number));
            }
            std::ostringstream ss;
            ss << m_number;
            return ss.str();
        }
        case ValueType::String: return m_string;
        case ValueType::Object: return "[object Object]";
        case ValueType::Array: {
            if (!m_array) return "";
            std::string res;
            for (size_t i = 0; i < m_array->elements.size(); ++i) {
                if (i > 0) res += ",";
                res += m_array->elements[i].toString();
            }
            return res;
        }
        case ValueType::Function: {
            if (!m_function) return "function() {}";
            return "function " + m_function->name + "() { [native code] }";
        }
    }
    return "";
}

std::string Value::typeOf() const noexcept {
    switch (m_type) {
        case ValueType::Undefined: return "undefined";
        case ValueType::Null: return "object"; // JS standard quirk
        case ValueType::Boolean: return "boolean";
        case ValueType::Number: return "number";
        case ValueType::String: return "string";
        case ValueType::Object:
        case ValueType::Array: return "object";
        case ValueType::Function: return "function";
    }
    return "undefined";
}

Value Value::getProperty(const std::string& key) const {
    if (m_type == ValueType::Object && m_object) {
        auto it = m_object->properties.find(key);
        if (it != m_object->properties.end()) {
            return it->second;
        }
    } else if (m_type == ValueType::Array && m_array) {
        if (key == "length") {
            return Value(static_cast<double>(m_array->elements.size()));
        }
        try {
            size_t idx = std::stoul(key);
            if (idx < m_array->elements.size()) {
                return m_array->elements[idx];
            }
        } catch (...) {}
    } else if (m_type == ValueType::String) {
        if (key == "length") {
            return Value(static_cast<double>(m_string.size()));
        }
    }
    return Value(); // undefined
}

void Value::setProperty(const std::string& key, Value val) {
    if (m_type == ValueType::Object) {
        if (!m_object) m_object = std::make_shared<ObjectData>();
        m_object->properties[key] = std::move(val);
    } else if (m_type == ValueType::Array) {
        if (!m_array) m_array = std::make_shared<ArrayData>();
        try {
            size_t idx = std::stoul(key);
            if (idx >= m_array->elements.size()) {
                m_array->elements.resize(idx + 1);
            }
            m_array->elements[idx] = std::move(val);
        } catch (...) {
            // Non-numeric properties on array ignored or supported
        }
    }
}

bool Value::hasProperty(const std::string& key) const {
    if (m_type == ValueType::Object && m_object) {
        return m_object->properties.contains(key);
    }
    if (m_type == ValueType::Array && m_array) {
        if (key == "length") return true;
        try {
            size_t idx = std::stoul(key);
            return idx < m_array->elements.size();
        } catch (...) {}
    }
    return false;
}

size_t Value::length() const {
    if (m_type == ValueType::Array && m_array) return m_array->elements.size();
    if (m_type == ValueType::String) return m_string.size();
    return 0;
}

Value Value::getIndex(size_t index) const {
    if (m_type == ValueType::Array && m_array && index < m_array->elements.size()) {
        return m_array->elements[index];
    }
    return Value();
}

void Value::setIndex(size_t index, Value val) {
    if (m_type == ValueType::Array) {
        if (!m_array) m_array = std::make_shared<ArrayData>();
        if (index >= m_array->elements.size()) {
            m_array->elements.resize(index + 1);
        }
        m_array->elements[index] = std::move(val);
    }
}

void Value::push(Value val) {
    if (m_type == ValueType::Array) {
        if (!m_array) m_array = std::make_shared<ArrayData>();
        m_array->elements.push_back(std::move(val));
    }
}

Value Value::call(const std::vector<Value>& args) const {
    if (m_type == ValueType::Function && m_function && m_function->isNative && m_function->native) {
        return m_function->native(args);
    }
    return Value();
}

bool Value::equals(const Value& other) const {
    if (m_type == other.m_type) {
        return strictEquals(other);
    }
    // Loose equality coercions
    if ((isUndefined() && other.isNull()) || (isNull() && other.isUndefined())) {
        return true;
    }
    if (isNumber() && other.isString()) {
        return m_number == other.toNumber();
    }
    if (isString() && other.isNumber()) {
        return toNumber() == other.m_number;
    }
    if (isBoolean()) {
        return Value(toNumber()).equals(other);
    }
    if (other.isBoolean()) {
        return equals(Value(other.toNumber()));
    }
    return false;
}

bool Value::strictEquals(const Value& other) const {
    if (m_type != other.m_type) return false;
    switch (m_type) {
        case ValueType::Undefined:
        case ValueType::Null:
            return true;
        case ValueType::Boolean:
            return m_bool == other.m_bool;
        case ValueType::Number:
            if (std::isnan(m_number) || std::isnan(other.m_number)) return false;
            return m_number == other.m_number;
        case ValueType::String:
            return m_string == other.m_string;
        case ValueType::Object:
            return m_object == other.m_object;
        case ValueType::Array:
            return m_array == other.m_array;
        case ValueType::Function:
            return m_function == other.m_function;
    }
    return false;
}

std::ostream& operator<<(std::ostream& os, const Value& val) {
    return os << val.toString();
}

} // namespace browser::javascript

