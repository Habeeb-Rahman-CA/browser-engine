#pragma once

#include "browser/dom/element.hpp"
#include "browser/css/stylesheet.hpp"

namespace browser::style {

class SelectorMatcher {
public:
    // Check if an element matches a complex selector (supporting child & descendant combinators)
    [[nodiscard]] static bool matches(const dom::Element& element, const css::ComplexSelector& selector);

    // Check if an element matches a single compound selector
    [[nodiscard]] static bool matchesCompound(const dom::Element& element, const css::CompoundSelector& compound);

    // Check if an element matches a simple selector
    [[nodiscard]] static bool matchesSimple(const dom::Element& element, const css::SimpleSelector& simple);
};

} // namespace browser::style

