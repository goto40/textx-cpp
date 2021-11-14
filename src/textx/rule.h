#pragma once

#include "arpeggio.h"
#include <unordered_map>
#include <string>
#include <variant>
#include <vector>

namespace textx {

    class Rule {
        textx::arpeggio::Pattern pattern;
        Rule() = default;
    public:

        std::optional<textx::arpeggio::Match> operator()(const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) {
            return pattern(config, text, pos);
        }

        friend Rule createRuleFromTextxPattern(std::string_view name, textx::arpeggio::Match rule_params, textx::arpeggio::Match rule_body);
    };
    Rule createRuleFromTextxPattern(std::string_view name, textx::arpeggio::Match rule_params, textx::arpeggio::Match rule_body);
}