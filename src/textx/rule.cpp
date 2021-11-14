#include "textx/rule.h"
#include <unordered_map>

namespace {
    std::unordered_map<std::string, std::function<textx::arpeggio::Pattern()>> transform_match2rule = {
        {
            "textx_rule_body",
            [](){
                return textx::arpeggio::str_match("test");
            }
        },
    };
}

namespace textx {

    Rule createRuleFromTextxPattern(std::string_view name, textx::arpeggio::Match rule_params, textx::arpeggio::Match rule_body) {
        //std::cout << rule_body << "\n";
        Rule rule;
        auto pattern = textx::arpeggio::str_match("");
        rule.pattern = pattern;
        return rule;
    }

}