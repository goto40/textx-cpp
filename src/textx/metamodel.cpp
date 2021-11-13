#include "textx/metamodel.h"
#include <cassert>

namespace textx {

    Metamodel::Metamodel(std::string_view grammar) {
        grammar_root = textx_grammar.parse_or_throw(grammar);
        auto &root = grammar_root.value();

        assert(root.name && root.name.value()=="textx_model" && "unexpected: no textx model loaded!!");

        auto &rules = root.children[1];
        for (auto&r : rules.children) {
            auto &rule_name = r.children[0].captured.value();
            //grammar.add(rule_name,);
        }
    }


}