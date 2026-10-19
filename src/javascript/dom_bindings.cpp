#include "browser/javascript/dom_bindings.hpp"
#include "browser/dom/text_node.hpp"
#include "browser/html/parser.hpp"
#include "browser/core/diagnostics.hpp"

#include <iostream>

namespace browser::javascript {

DomBindings::DomBindings(Interpreter& interpreter, std::shared_ptr<dom::Document> document)
    : m_interpreter(interpreter), m_document(std::move(document)) {
    setupGlobalObjects();
}

void DomBindings::bindDocument(std::shared_ptr<dom::Document> document) {
    m_document = std::move(document);
    m_listeners.clear();
    m_mutated = false;
    setupGlobalObjects();
}

void DomBindings::setupGlobalObjects() {
    // Window object
    auto windowObj = Value::makeObject();

    // Document object
    auto docObj = Value::makeObject();
    docObj.setProperty("title", Value(m_document ? m_document->title() : ""));

    // document.getElementById(id)
    docObj.setProperty("getElementById", Value([this](const std::vector<Value>& args) -> Value {
        if (args.empty() || !m_document) return Value(nullptr);
        std::string id = args[0].toString();
        auto elem = m_document->getElementById(id);
        if (!elem) return Value(nullptr);
        return wrapNode(elem);
    }, "getElementById"));

    // document.getElementsByTagName(tag)
    docObj.setProperty("getElementsByTagName", Value([this](const std::vector<Value>& args) -> Value {
        if (args.empty() || !m_document) return Value::makeArray();
        std::string tag = args[0].toString();
        auto elements = m_document->getElementsByTagName(tag);
        std::vector<Value> arr;
        for (const auto& el : elements) {
            arr.push_back(wrapNode(el));
        }
        return Value::makeArray(std::move(arr));
    }, "getElementsByTagName"));

    // document.createElement(tagName)
    docObj.setProperty("createElement", Value([this](const std::vector<Value>& args) -> Value {
        if (args.empty() || !m_document) return Value(nullptr);
        std::string tag = args[0].toString();
        auto elem = m_document->createElement(tag);
        return wrapNode(elem);
    }, "createElement"));

    // document.createTextNode(text)
    docObj.setProperty("createTextNode", Value([this](const std::vector<Value>& args) -> Value {
        if (!m_document) return Value(nullptr);
        std::string text = args.empty() ? "" : args[0].toString();
        auto textNode = m_document->createTextNode(text);
        return wrapNode(textNode);
    }, "createTextNode"));

    // document.body
    if (m_document && m_document->body()) {
        docObj.setProperty("body", wrapNode(m_document->body()));
    }

    // document.addEventListener(type, fn)
    docObj.setProperty("addEventListener", Value([this](const std::vector<Value>& args) -> Value {
        if (args.size() < 2 || !m_document) return Value();
        std::string eventName = args[0].toString();
        Value callback = args[1];
        if (callback.isFunction()) {
            m_listeners[m_document.get()][eventName].push_back(std::move(callback));
        }
        return Value();
    }, "addEventListener"));

    windowObj.setProperty("document", docObj);
    windowObj.setProperty("window", windowObj);

    // Register globals
    m_interpreter.setGlobal("window", windowObj);
    m_interpreter.setGlobal("document", docObj);
}

Value DomBindings::wrapNode(const std::shared_ptr<dom::Node>& node) {
    if (!node) return Value(nullptr);

    auto nodeObj = Value::makeObject();
    nodeObj.objectData()->hostPointer = node.get();

    nodeObj.setProperty("nodeType", Value(static_cast<double>(static_cast<int>(node->nodeType()))));
    nodeObj.setProperty("nodeName", Value(node->nodeName()));
    nodeObj.setProperty("textContent", Value(node->textContent()));

    // node.setTextContent(text)
    nodeObj.setProperty("setTextContent", Value([this, node](const std::vector<Value>& args) -> Value {
        if (!args.empty()) {
            node->setTextContent(args[0].toString());
            m_mutated = true;
        }
        return Value();
    }, "setTextContent"));

    // node.getTextContent()
    nodeObj.setProperty("getTextContent", Value([node](const std::vector<Value>&) -> Value {
        return Value(node->textContent());
    }, "getTextContent"));

    // node.appendChild(childNode)
    nodeObj.setProperty("appendChild", Value([this, node](const std::vector<Value>& args) -> Value {
        if (args.empty() || !args[0].isObject()) return Value();
        auto childData = args[0].objectData();
        if (childData && childData->hostPointer) {
            auto* childPtr = static_cast<dom::Node*>(childData->hostPointer);
            node->appendChild(childPtr->shared_from_this());
            m_mutated = true;
        }
        return args[0];
    }, "appendChild"));

    // node.removeChild(childNode)
    nodeObj.setProperty("removeChild", Value([this, node](const std::vector<Value>& args) -> Value {
        if (args.empty() || !args[0].isObject()) return Value();
        auto childData = args[0].objectData();
        if (childData && childData->hostPointer) {
            auto* childPtr = static_cast<dom::Node*>(childData->hostPointer);
            node->removeChild(childPtr->shared_from_this());
            m_mutated = true;
        }
        return args[0];
    }, "removeChild"));

    // node.addEventListener(event, callback)
    nodeObj.setProperty("addEventListener", Value([this, node](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) return Value();
        std::string eventName = args[0].toString();
        Value callback = args[1];
        if (callback.isFunction()) {
            m_listeners[node.get()][eventName].push_back(std::move(callback));
        }
        return Value();
    }, "addEventListener"));

    if (node->nodeType() == dom::NodeType::Element) {
        auto elem = std::static_pointer_cast<dom::Element>(node);
        nodeObj.setProperty("tagName", Value(elem->tagName()));
        nodeObj.setProperty("id", Value(elem->id()));
        nodeObj.setProperty("className", Value(elem->className()));

        // elem.getAttribute(name)
        nodeObj.setProperty("getAttribute", Value([elem](const std::vector<Value>& args) -> Value {
            if (args.empty()) return Value(nullptr);
            std::string name = args[0].toString();
            if (elem->hasAttribute(name)) {
                return Value(elem->getAttribute(name));
            }
            return Value(nullptr);
        }, "getAttribute"));

        // elem.setAttribute(name, value)
        nodeObj.setProperty("setAttribute", Value([this, elem](const std::vector<Value>& args) -> Value {
            if (args.size() >= 2) {
                elem->setAttribute(args[0].toString(), args[1].toString());
                m_mutated = true;
            }
            return Value();
        }, "setAttribute"));

        // elem.hasAttribute(name)
        nodeObj.setProperty("hasAttribute", Value([elem](const std::vector<Value>& args) -> Value {
            if (args.empty()) return Value(false);
            return Value(elem->hasAttribute(args[0].toString()));
        }, "hasAttribute"));

        // elem.removeAttribute(name)
        nodeObj.setProperty("removeAttribute", Value([this, elem](const std::vector<Value>& args) -> Value {
            if (!args.empty()) {
                elem->removeAttribute(args[0].toString());
                m_mutated = true;
            }
            return Value();
        }, "removeAttribute"));

        // elem.setInnerHTML(html)
        nodeObj.setProperty("setInnerHTML", Value([this, elem](const std::vector<Value>& args) -> Value {
            if (!args.empty()) {
                std::string markup = args[0].toString();
                auto parsedDoc = html::HtmlParser::parseDocument(markup);
                if (parsedDoc && parsedDoc->body()) {
                    // Clear current children
                    while (elem->hasChildNodes()) {
                        elem->removeChild(elem->childNodes().front());
                    }
                    auto childrenCopy = parsedDoc->body()->childNodes();
                    for (const auto& ch : childrenCopy) {
                        elem->appendChild(ch);
                    }
                    m_mutated = true;
                }
            }
            return Value();
        }, "setInnerHTML"));
    }

    return nodeObj;
}

bool DomBindings::dispatchEvent(const std::shared_ptr<dom::Node>& target, const std::string& eventType, Value eventObj) {
    if (!target) return false;

    // Traverse up from target to root (event bubbling)
    dom::Node* curr = target.get();
    bool handled = false;

    while (curr) {
        auto it = m_listeners.find(curr);
        if (it != m_listeners.end()) {
            auto eventIt = it->second.find(eventType);
            if (eventIt != it->second.end()) {
                for (const auto& listener : eventIt->second) {
                    m_interpreter.callFunction(listener, {eventObj});
                    handled = true;
                }
            }
        }
        auto parent = curr->parentNode();
        curr = parent ? parent.get() : nullptr;
    }

    return handled;
}

} // namespace browser::javascript

