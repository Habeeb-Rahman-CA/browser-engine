#pragma once

#include "browser/dom/node.hpp"

namespace browser::dom {

class TextNode : public Node {
public:
    explicit TextNode(std::string data);

    [[nodiscard]] const std::string& data() const noexcept { return m_data; }
    void setData(std::string_view data);

    [[nodiscard]] std::string textContent() const override { return m_data; }
    void setTextContent(std::string_view text) override { setData(text); }

private:
    std::string m_data;
};

} // namespace browser::dom
