#include "browser/dom/text_node.hpp"
#include "browser/dom/comment_node.hpp"

namespace browser::dom {

TextNode::TextNode(std::string data)
    : Node(NodeType::Text, "#text"), m_data(std::move(data)) {}

void TextNode::setData(std::string_view data) {
    m_data = std::string(data);
}

CommentNode::CommentNode(std::string data)
    : Node(NodeType::Comment, "#comment"), m_data(std::move(data)) {}

} // namespace browser::dom
