#pragma once

#include "arpeggio.h"
#include "grammar.h"
#include <unordered_map>
#include <string>
#include <variant>
#include <vector>

namespace textx {

    struct Rule {
        textx::arpeggio::Pattern pattern;

        std::optional<textx::arpeggio::Match> operator()(const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) {
            return pattern(config, text, pos);
        }
    };

    Rule createRuleFromTextxPattern(textx::Grammar<textx::Rule>& grammar, std::string_view name, textx::arpeggio::Match rule_params, textx::arpeggio::Match rule_body);
}