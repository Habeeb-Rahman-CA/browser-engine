#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <compare>
#include <string_view>

namespace browser::css {

struct Specificity {
    uint32_t a{0}; // IDs
    uint32_t b{0}; // Classes, attributes, pseudo-classes
    uint32_t c{0}; // Elements, pseudo-elements

    auto operator<=>(const Specificity& other) const = default;
};

enum class Combinator {
    None,
    Descendant, // ' '
    Child       // '>'
};

enum class SimpleSelectorType {
    Universal,   // *
    Type,        // div, p, h1
    Class,       // .container
    Id,          // #main
    PseudoClass  // :hover, :focus
};

struct SimpleSelector {
    SimpleSelectorType type{SimpleSelectorType::Universal};
    std::string value;
};

struct CompoundSelector {
    std::vector<SimpleSelector> simpleSelectors;

    [[nodiscard]] Specificity specificity() const noexcept {
        Specificity s;
        for (const auto& sel : simpleSelectors) {
            switch (sel.type) {
                case SimpleSelectorType::Id:          s.a++; break;
                case SimpleSelectorType::Class:
                case SimpleSelectorType::PseudoClass: s.b++; break;
                case SimpleSelectorType::Type:        s.c++; break;
                case SimpleSelectorType::Universal:   break;
            }
        }
        return s;
    }
};

struct SelectorPart {
    CompoundSelector compound;
    Combinator combinator{Combinator::None}; // Combinator to next part
};

struct ComplexSelector {
    std::vector<SelectorPart> parts;

    [[nodiscard]] Specificity specificity() const noexcept {
        Specificity total;
        for (const auto& part : parts) {
            auto partSpec = part.compound.specificity();
            total.a += partSpec.a;
            total.b += partSpec.b;
            total.c += partSpec.c;
        }
        return total;
    }
};

struct Declaration {
    std::string property;
    std::string value;
    bool important{false};
};

struct Rule {
    std::vector<ComplexSelector> selectors;
    std::vector<Declaration> declarations;
};

struct StyleSheet {
    std::vector<Rule> rules;
};

} // namespace browser::css
