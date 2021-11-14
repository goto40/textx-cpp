#include "textx/metamodel.h"
#include "textx/rule.h"
#include <cassert>

namespace textx {

    Metamodel::Metamodel(std::string_view grammar_text) {
        grammar_root = textx_grammar.parse_or_throw(grammar_text);        
        auto &root = grammar_root.value();

        assert(root.name && "unexpected: no textx model loaded!!");
        assert(root.name.value()=="textx_model" && "unexpected: no textx model loaded!!");

        //TODO import and refs
                
        auto &rules = root.children[1];
        std::cout << "children: " << rules.children.size() << "\n";

        bool first = true;
        for (auto&r : rules.children) {
            auto &rule_name = r.fw().children[0].fw().captured.value();
            std::cout << "r: " << rule_name << "\n";
            auto &rule_params = r.fw().children[1];
            auto &rule_body = r.fw().children[3];
            if (first) {
                grammar.set_main_rule(rule_name);
                first = false;
            }
            grammar.add_rule(rule_name, textx::createRuleFromTextxPattern(grammar, rule_name, rule_params, rule_body));
        }
    }


}