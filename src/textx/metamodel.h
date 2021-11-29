#pragma once
#include "textx/lang.h"
#include "textx/grammar.h"
#include "textx/rule.h"
#include "textx/parsetree.h"
#include "textx/model.h"
#include <string>
#include <memory>

namespace textx {

    class Metamodel : public std::enable_shared_from_this<Metamodel> {
        textx::lang::TextxGrammar textx_grammar={};
        textx::Grammar<textx::Rule> grammar={};
        textx::parsetree::ParseTree grammar_parsetree;
        static Metamodel& get_basic_metamodel();

        public:
        Metamodel(std::string_view grammar, bool is_main_grammar=true);

        std::optional<textx::arpeggio::Match> parsetree_from_str(std::string_view model_txt) {
            return grammar.parse_or_throw(model_txt);
        }

        Rule& operator[](std::string name) {
            return grammar[name];
        }

        const Rule& operator[](std::string name) const {
            return grammar[name];
        }

        textx::arpeggio::Pattern ref(std::string name) {
            return textx::arpeggio::rule(textx::arpeggio::named(name, [this,name](const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) -> std::optional<textx::arpeggio::Match>
            {
                auto r = grammar.get_rules().find(name);
                if (r==grammar.get_rules().end()) {
                    if (this != &get_basic_metamodel()) {
                        return get_basic_metamodel().ref(name)(config, text, pos);
                    }
                    else {
                        throw std::runtime_error(std::string("cannot find mm.ref(\"")+name+"\");");
                    }
                }
                else {
                    return r->second(config, text, pos);
                }
            }));

            //TODO handle referenced/included metamodels
            if (grammar.get_rules().find(name)==grammar.get_rules().end()) {
                std::cout << "?????? " << name << "\n";
                // if (this != &get_basic_metamodel()) {
                //     return get_basic_metamodel().ref(name);
                // }
            }
            return grammar.ref(name);
        }

        inline friend std::ostream& operator<<(std::ostream &o, const Metamodel& mm) {
            for(auto& [k,v]: mm.grammar.get_rules()) {
                //o << "RULE[\"" << k << "\"]:\n";
                o << v << "\n";
            }
            return o;
        }

        std::shared_ptr<textx::Model> model_from_str(std::string_view text) {
            auto parsetree = parsetree_from_str(text);
            return std::make_shared<textx::Model>(*parsetree, shared_from_this());
        }

    };

}