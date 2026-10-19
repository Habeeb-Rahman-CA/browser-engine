#pragma once

#include "browser/dom/node.hpp"

namespace browser::dom {

class CommentNode : public Node {
public:
    explicit CommentNode(std::string data);

    [[nodiscard]] const std::string& data() const noexcept { return m_data; }
    void setData(std::string_view data) { m_data = std::string(data); }

    [[nodiscard]] std::string textContent() const override { return m_data; }

private:
    std::string m_data;
};

} // namespace browser::dom

