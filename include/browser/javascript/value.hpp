#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <variant>
#include <ostream>

namespace browser::javascript {

class Environment;
class Statement;
class FunctionStmt;

enum class ValueType {
    Undefined,
    Null,
    Boolean,
    Number,
    String,
    Object,
    Array,
    Function
};

class Value;
struct ObjectData;
struct ArrayData;
struct FunctionData;

using NativeFunction = std::function<Value(const std::vector<Value>&)>;

class Value {
public:
    Value(); // Undefined
    Value(std::nullptr_t); // Null
    Value(bool b);
    Value(double d);
    Value(int i);
    Value(size_t s);
    Value(const char* s);
    Value(std::string s);
    Value(NativeFunction fn, std::string name = "native");
    Value(std::shared_ptr<ObjectData> obj);
    Value(std::shared_ptr<ArrayData> arr);
    Value(std::shared_ptr<FunctionData> fn);

    static Value makeObject();
    static Value makeArray(std::vector<Value> items = {});

    [[nodiscard]] ValueType type() const noexcept { return m_type; }
    [[nodiscard]] bool isUndefined() const noexcept { return m_type == ValueType::Undefined; }
    [[nodiscard]] bool isNull() const noexcept { return m_type == ValueType::Null; }
    [[nodiscard]] bool isBoolean() const noexcept { return m_type == ValueType::Boolean; }
    [[nodiscard]] bool isNumber() const noexcept { return m_type == ValueType::Number; }
    [[nodiscard]] bool isString() const noexcept { return m_type == ValueType::String; }
    [[nodiscard]] bool isObject() const noexcept { return m_type == ValueType::Object; }
    [[nodiscard]] bool isArray() const noexcept { return m_type == ValueType::Array; }
    [[nodiscard]] bool isFunction() const noexcept { return m_type == ValueType::Function; }

    // Conversions
    [[nodiscard]] bool toBoolean() const noexcept;
    [[nodiscard]] double toNumber() const noexcept;
    [[nodiscard]] std::string toString() const;
    [[nodiscard]] std::string typeOf() const noexcept;

    // Direct accessors (with fallback defaults)
    [[nodiscard]] double asNumber() const noexcept { return m_number; }
    [[nodiscard]] bool asBoolean() const noexcept { return m_bool; }
    [[nodiscard]] const std::string& asString() const noexcept { return m_string; }

    // Object and array properties
    [[nodiscard]] Value getProperty(const std::string& key) const;
    void setProperty(const std::string& key, Value val);
    [[nodiscard]] bool hasProperty(const std::string& key) const;

    // Array operations
    [[nodiscard]] size_t length() const;
    [[nodiscard]] Value getIndex(size_t index) const;
    void setIndex(size_t index, Value val);
    void push(Value val);

    // Call function
    [[nodiscard]] Value call(const std::vector<Value>& args) const;

    // Comparison
    [[nodiscard]] bool equals(const Value& other) const;
    [[nodiscard]] bool strictEquals(const Value& other) const;

    // Internal data access
    [[nodiscard]] std::shared_ptr<ObjectData> objectData() const noexcept { return m_object; }
    [[nodiscard]] std::shared_ptr<ArrayData> arrayData() const noexcept { return m_array; }
    [[nodiscard]] std::shared_ptr<FunctionData> functionData() const noexcept { return m_function; }

private:
    ValueType m_type{ValueType::Undefined};
    bool m_bool{false};
    double m_number{0.0};
    std::string m_string;
    std::shared_ptr<ObjectData> m_object;
    std::shared_ptr<ArrayData> m_array;
    std::shared_ptr<FunctionData> m_function;
};

struct ObjectData {
    std::unordered_map<std::string, Value> properties;
    void* hostPointer{nullptr}; // Can hold pointer to host DOM Node or Document
};

struct ArrayData {
    std::vector<Value> elements;
};

struct FunctionData {
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<Statement> body;
    std::shared_ptr<Environment> closure;
    NativeFunction native;
    bool isNative{false};
};

std::ostream& operator<<(std::ostream& os, const Value& val);

} // namespace browser::javascript

