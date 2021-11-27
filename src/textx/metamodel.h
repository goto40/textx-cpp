#pragma once
#include "textx/lang.h"
#include "textx/grammar.h"
#include "textx/rule.h"
#include "textx/parsetree.h"
#include <string>

namespace textx {

    class Metamodel {
        textx::lang::TextxGrammar textx_grammar={};
        textx::Grammar<textx::Rule> grammar={};
        textx::parsetree::ParseTree grammar_parsetree;

        public:
        Metamodel(std::string_view grammar);

        std::optional<textx::arpeggio::Match> parsetree_from_str(std::string_view model_txt) {
            return grammar.parse_or_throw(model_txt);
        }

        // std::optional<textx::arpeggio::Match> model_from_str(std::string_view model_txt) {
        //     auto parsetree = parsetree_from_str(model_txt);
        //     return xxx;
        // }

        Rule& operator[](std::string name) {
            return grammar[name];
        }

        const Rule& operator[](std::string name) const {
            return grammar[name];
        }

        auto ref(std::string name) {
            //TODO handle referenced/included metamodels
            return grammar.ref(name);
        }

        inline friend std::ostream& operator<<(std::ostream &o, const Metamodel& mm) {
            for(auto& [k,v]: mm.grammar.get_rules()) {
                //o << "RULE[\"" << k << "\"]:\n";
                o << v << "\n";
            }
            return o;
        }

    };

}