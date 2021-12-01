#pragma once
#include "textx/lang.h"
#include "textx/grammar.h"
#include "textx/rule.h"
#include "textx/textx_grammar_parsetree.h"
#include "textx/model.h"
#include <string>
#include <memory>

namespace textx::scoping {
    struct RefResolver {
        /** looks for obj_name in attr_name, starting form origin */
        virtual std::shared_ptr<textx::object::Object> resolve(std::shared_ptr<textx::object::Object> origin, std::string attr_name, std::string obj_name)=0;
        virtual ~RefResolver() = default;
    };

    struct DefaultRefResolver : RefResolver {
        std::shared_ptr<textx::object::Object> resolve(std::shared_ptr<textx::object::Object> origin, std::string attr_name, std::string obj_name) override {
            auto m = origin->tx_model.lock();

            std::function<std::shared_ptr<textx::object::Object>(textx::object::Value&)> traverse;
            traverse = [&, this](textx::object::Value& v) -> std::shared_ptr<textx::object::Object> {
                if (v.is_str()) {
                    // nothing
                }
                else if (v.is_ref()) {
                    // nothing
                }
                else if (v.is_pure_obj()) {
                    // std::cout << "resolve ... " << v.obj()->type << "\n";
                    // if (v.obj()->has_attr("name")) {
                    //     std::cout << "name ='" <<(*v.obj())["name"].str()<< "' ==? '" << obj_name<<"'\n";
                    // }
                    if (v.obj()->has_attr("name") && (*v.obj())["name"].str()==obj_name) {
                        //std::cout << "FOUND!\n";
                        return v.obj();
                    } 
                    for (auto &[k,av]: v.obj()->attributes) {
                        if (std::holds_alternative<textx::object::Value>(av.data)) {
                            auto p = traverse(std::get<textx::object::Value>(av.data));
                            if (p) return p;
                        }
                        else {
                            for (auto &iv: std::get<std::vector<textx::object::Value>>(av.data)) {
                                auto p = traverse(iv);
                                if (p) return p;
                            }
                        }
                    }
                }
                else {
                    textx::arpeggio::raise(v.pos, "unexpected situaltion");
                }
                return nullptr;
            };
            return traverse(m->val());
        }
    };
}

namespace textx {
    class Metamodel : public std::enable_shared_from_this<Metamodel> {
        textx::lang::TextxGrammar textx_grammar={};
        textx::Grammar<textx::Rule> grammar={};
        textx::parsetree::TextxGrammarParsetree textx_grammar_parsetree;
        static Metamodel& get_basic_metamodel();
        std::unique_ptr<textx::scoping::RefResolver> default_resolver = std::make_unique<textx::scoping::DefaultRefResolver>();

        public:
        Metamodel(std::string_view grammar, bool is_main_grammar=true, bool include_basic_metamodel=true);
        Rule& operator[](std::string name);
        const Rule& operator[](std::string name) const;
        textx::arpeggio::Pattern ref(std::string name);
        std::shared_ptr<textx::Model> model_from_str(std::string_view text);

        textx::scoping::RefResolver& get_resolver(std::string rule_name, std::string attr_name) {
            //TODO select registered resolver
            return *default_resolver;
        }

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