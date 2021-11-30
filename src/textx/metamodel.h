#pragma once
#include "textx/lang.h"
#include "textx/grammar.h"
#include "textx/rule.h"
#include "textx/textx_grammar_parsetree.h"
#include "textx/model.h"
#include <string>
#include <memory>

namespace textx {

    class Metamodel : public std::enable_shared_from_this<Metamodel> {
        textx::lang::TextxGrammar textx_grammar={};
        textx::Grammar<textx::Rule> grammar={};
        textx::parsetree::TextxGrammarParsetree textx_grammar_parsetree;
        static Metamodel& get_basic_metamodel();

        public:
        Metamodel(std::string_view grammar, bool is_main_grammar=true, bool include_basic_metamodel=true);
        Rule& operator[](std::string name);
        const Rule& operator[](std::string name) const;
        textx::arpeggio::Pattern ref(std::string name);
        std::shared_ptr<textx::Model> model_from_str(std::string_view text);

        std::optional<textx::arpeggio::Match> parsetree_from_str(std::string_view model_txt) { return grammar.parse_or_throw(model_txt); }
        inline friend std::ostream& operator<<(std::ostream &o, const Metamodel& mm) {
            for(auto& [k,v]: mm.grammar.get_rules()) {
                //o << "RULE[\"" << k << "\"]:\n";
                o << v << "\n";
            }
            return o;
        }
    };

}