#pragma once

#include "browser/dom/node.hpp"
#include <unordered_map>
#include <optional>

namespace browser::dom {

class Element : public Node {
public:
    explicit Element(std::string tagName);

    [[nodiscard]] const std::string& tagName() const noexcept { return m_nodeName; }

    // Attributes
    [[nodiscard]] bool hasAttribute(std::string_view name) const noexcept;
    [[nodiscard]] std::string getAttribute(std::string_view name) const noexcept;
    void setAttribute(std::string_view name, std::string_view value);
    void removeAttribute(std::string_view name);
    [[nodiscard]] const std::unordered_map<std::string, std::string>& attributes() const noexcept { return m_attributes; }

    // Convenience properties
    [[nodiscard]] std::string id() const noexcept;
    [[nodiscard]] std::string className() const noexcept;
    [[nodiscard]] std::vector<std::string> classList() const;

    // Queries
    [[nodiscard]] std::shared_ptr<Element> getElementById(std::string_view id) const;
    [[nodiscard]] std::vector<std::shared_ptr<Element>> getElementsByTagName(std::string_view tagName) const;

    // Content
    [[nodiscard]] std::string textContent() const override;
    void setTextContent(std::string_view text) override;

private:
    std::unordered_map<std::string, std::string> m_attributes;
};

} // namespace browser::dom

