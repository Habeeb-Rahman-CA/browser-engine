#pragma once

#include "browser/javascript/interpreter.hpp"
#include "browser/dom/document.hpp"
#include "browser/dom/element.hpp"

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

namespace browser::javascript {

class DomBindings {
public:
    DomBindings(Interpreter& interpreter, std::shared_ptr<dom::Document> document);

    // Re-bind to a new document (e.g. on navigation)
    void bindDocument(std::shared_ptr<dom::Document> document);

    // Create a JS Value wrapping a DOM Node / Element
    Value wrapNode(const std::shared_ptr<dom::Node>& node);

    // Dispatch DOM event to registered listeners
    bool dispatchEvent(const std::shared_ptr<dom::Node>& target, const std::string& eventType, Value eventObj = Value());

    // Check if DOM was mutated by script execution
    [[nodiscard]] bool hasMutations() const noexcept { return m_mutated; }
    void clearMutations() noexcept { m_mutated = false; }

private:
    void setupGlobalObjects();

    Interpreter& m_interpreter;
    std::shared_ptr<dom::Document> m_document;
    bool m_mutated{false};

    // Stored event listeners: Node raw pointer -> eventName -> list of JS callback Functions
    std::unordered_map<const dom::Node*, std::unordered_map<std::string, std::vector<Value>>> m_listeners;
};

} // namespace browser::javascript

