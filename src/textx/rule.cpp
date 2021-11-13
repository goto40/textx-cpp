#include "textx/rule.h"
#include <unordered_map>

namespace {
    std::unordered_map<std::string, std::function<textx::arpeggio::Pattern()>> transform_match2rule = {
        {
            "textx_model",
            [](){
                return textx::arpeggio::str_match("test");
            }
        },
    };
}

namespace textx {

    Rule createRuleFromTextxPattern(textx::arpeggio::Match m) {
        auto p = textx::arpeggio::str_match("");
        return Rule(p);        
    }

}