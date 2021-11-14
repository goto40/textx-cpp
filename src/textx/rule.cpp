#include "textx/rule.h"
#include <unordered_map>

namespace {
    using RULE = textx::Rule;
    using GRAMMAR = textx::Grammar<RULE>;

    textx::arpeggio::Pattern transform_match2pattern(GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match);

    std::unordered_map<std::string, std::function<textx::arpeggio::Pattern(GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match)>> transform_match2pattern_map = {
        {
            "textx_rule_body",
            [](GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match){
                return transform_match2pattern(grammar, rule, match.children[0]);
            }
        },
    };

    textx::arpeggio::Pattern transform_match2pattern(GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) {
        if (transform_match2pattern_map.count(match.name.value())==1) {
            return transform_match2pattern_map[match.name.value()](grammar, rule, match);
        }
        else {
            throw std::runtime_error(std::string("unexpected: no entry in transform_match2pattern_map for ")+match.name.value());
        }
    }
}

namespace textx {

    Rule createRuleFromTextxPattern(textx::Grammar<textx::Rule>& grammar, std::string_view name, textx::arpeggio::Match rule_params, const textx::arpeggio::Match& rule_body) {
        std::cout << rule_body << "\n";
        Rule rule;
        rule.pattern = transform_match2pattern(grammar, rule, rule_body);
        return rule;
    }

}