#include "browser/style/selector_matcher.hpp"

#include <algorithm>

namespace browser::style {

bool SelectorMatcher::matchesSimple(const dom::Element& element, const css::SimpleSelector& simple) {
    switch (simple.type) {
        case css::SimpleSelectorType::Universal:
            return true;

        case css::SimpleSelectorType::Type:
            return simple.value == "*" || element.tagName() == simple.value;

        case css::SimpleSelectorType::Class: {
            auto classes = element.classList();
            return std::find(classes.begin(), classes.end(), simple.value) != classes.end();
        }

        case css::SimpleSelectorType::Id:
            return element.id() == simple.value;

        case css::SimpleSelectorType::PseudoClass:
            // Basic pseudo-class support: e.g. root, hover, focus
            if (simple.value == "root") {
                return element.tagName() == "html";
            }
            return false;
    }
    return false;
}

bool SelectorMatcher::matchesCompound(const dom::Element& element, const css::CompoundSelector& compound) {
    if (compound.simpleSelectors.empty()) {
        return false;
    }
    for (const auto& simple : compound.simpleSelectors) {
        if (!matchesSimple(element, simple)) {
            return false;
        }
    }
    return true;
}

bool SelectorMatcher::matches(const dom::Element& element, const css::ComplexSelector& selector) {
    if (selector.parts.empty()) {
        return false;
    }

    // Rightmost part must match the element
    size_t partIdx = selector.parts.size() - 1;
    if (!matchesCompound(element, selector.parts[partIdx].compound)) {
        return false;
    }

    if (partIdx == 0) {
        return true;
    }

    // Walk ancestors to match remaining parts backwards
    std::shared_ptr<dom::Node> currentNode = element.parentNode();

    while (partIdx > 0 && currentNode) {
        if (currentNode->nodeType() != dom::NodeType::Element) {
            currentNode = currentNode->parentNode();
            continue;
        }

        auto currentElem = std::static_pointer_cast<dom::Element>(currentNode);
        css::Combinator comb = selector.parts[partIdx - 1].combinator;

        if (comb == css::Combinator::Child) {
            // Must match immediate parent
            if (!matchesCompound(*currentElem, selector.parts[partIdx - 1].compound)) {
                return false;
            }
            partIdx--;
            currentNode = currentNode->parentNode();
        } else {
            // Descendant: match any ancestor
            bool foundAncestor = false;
            while (currentNode) {
                if (currentNode->nodeType() == dom::NodeType::Element) {
                    auto ancestorElem = std::static_pointer_cast<dom::Element>(currentNode);
                    if (matchesCompound(*ancestorElem, selector.parts[partIdx - 1].compound)) {
                        foundAncestor = true;
                        partIdx--;
                        currentNode = currentNode->parentNode();
                        break;
                    }
                }
                currentNode = currentNode->parentNode();
            }
            if (!foundAncestor) {
                return false;
            }
        }
    }

    return partIdx == 0;
}

} // namespace browser::style

