#pragma once

#include "browser/layout/dimensions.hpp"
#include "browser/dom/node.hpp"
#include "browser/style/computed_style.hpp"

#include <vector>
#include <memory>
#include <string_view>
#include <iosfwd>

namespace browser::layout {

enum class BoxType {
    BlockNode,
    InlineNode,
    AnonymousBlock
};

[[nodiscard]] std::string_view boxTypeToString(BoxType type) noexcept;

class LayoutBox {
public:
    explicit LayoutBox(BoxType type, std::shared_ptr<style::ComputedStyle> style = nullptr, std::shared_ptr<dom::Node> node = nullptr);
    ~LayoutBox() = default;

    LayoutBox(const LayoutBox&) = delete;
    LayoutBox& operator=(const LayoutBox&) = delete;
    LayoutBox(LayoutBox&&) noexcept = default;
    LayoutBox& operator=(LayoutBox&&) noexcept = default;

    [[nodiscard]] BoxType boxType() const noexcept { return m_boxType; }
    [[nodiscard]] std::shared_ptr<dom::Node> node() const noexcept { return m_node; }
    [[nodiscard]] std::shared_ptr<style::ComputedStyle> style() const noexcept { return m_style; }
    void setStyle(std::shared_ptr<style::ComputedStyle> style) noexcept { m_style = std::move(style); }

    [[nodiscard]] BoxDimensions& dimensions() noexcept { return m_dimensions; }
    [[nodiscard]] const BoxDimensions& dimensions() const noexcept { return m_dimensions; }

    [[nodiscard]] LayoutBox* parent() const noexcept { return m_parent; }
    [[nodiscard]] const std::vector<std::unique_ptr<LayoutBox>>& children() const noexcept { return m_children; }

    void addChild(std::unique_ptr<LayoutBox> child);

    // Text content for inline text nodes
    [[nodiscard]] const std::string& text() const noexcept { return m_text; }
    void setText(std::string text) { m_text = std::move(text); }

    // Tree inspection
    void dumpLayoutTree(std::ostream& out, int depth = 0) const;

private:
    BoxType m_boxType;
    std::shared_ptr<dom::Node> m_node;
    std::shared_ptr<style::ComputedStyle> m_style;
    BoxDimensions m_dimensions;
    LayoutBox* m_parent{nullptr};
    std::vector<std::unique_ptr<LayoutBox>> m_children;
    std::string m_text;
};

} // namespace browser::layout

