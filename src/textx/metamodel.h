#pragma once
#include "textx/lang.h"
#include "textx/grammar.h"
#include "textx/rule.h"
#include "textx/textx_grammar_parsetree.h"
#include "textx/model.h"
#include "textx/scoping.h"
#include <string>
#include <memory>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace textx {
    class Workspace;
    class Metamodel : public std::enable_shared_from_this<Metamodel> {
        textx::lang::TextxGrammar textx_grammar={};
        textx::Grammar<textx::Rule> grammar={};
        textx::parsetree::TextxGrammarParsetree textx_grammar_parsetree;
        static Metamodel& get_basic_metamodel();
        std::unique_ptr<textx::scoping::RefResolver> default_resolver = std::make_unique<textx::scoping::PlainNameRefResolver>();
        std::unordered_map<std::string, std::unique_ptr<textx::scoping::RefResolver>> resolver = {};
        std::vector<std::shared_ptr<textx::Model>> builtin_models={};
        std::shared_ptr<textx::Workspace> default_workspace;
        
        public:
        Metamodel(std::string_view grammar, bool is_main_grammar=true, bool include_basic_metamodel=true, std::string filename="");
        std::shared_ptr<textx::Workspace> tx_default_workspace();
        bool is_base_of(std::string t1, std::string t2) const;
        Rule& operator[](std::string name);
        const Rule& operator[](std::string name) const;
        bool has_rule(std::string name) const;
        textx::arpeggio::Pattern ref(std::string name);

        std::shared_ptr<textx::Model> model_from_str(std::string_view text, std::string filename="", bool is_main_model=true, std::shared_ptr<textx::Workspace> workspace=nullptr);
        std::shared_ptr<textx::Model> model_from_file(std::filesystem::path p, bool is_main_model=true, std::shared_ptr<textx::Workspace> workspace=nullptr);

        //const auto& tx_builtin_models() const { return builtin_models; }
        void add_builtin_model(std::shared_ptr<textx::Model> m) { builtin_models.push_back(std::move(m)); }

        const textx::scoping::RefResolver& get_resolver(std::string rule_name, std::string attr_name) const {
            std::string lookup[] = {
                rule_name+"."+attr_name,
                std::string("*.")+attr_name,
                rule_name+".*",
                "*.*"
            };
            for (auto l: lookup) {
                auto res = resolver.find(l);
                if (res!=resolver.end()) return *res->second;
            }
            return *default_resolver;
        }

        void set_resolver(std::string dot_separated_rule_attr_with_asterix, std::unique_ptr<textx::scoping::RefResolver> r) {
            resolver.insert({dot_separated_rule_attr_with_asterix, std::move(r)});
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

    inline auto metamodel_from_str(std::string_view grammar) {
        return std::make_shared<textx::Metamodel>(grammar);
    }

    inline auto metamodel_from_file(std::filesystem::path p) {
        std::ifstream file(p);
        std::stringstream grammartext;
        grammartext << file.rdbuf();
        return std::make_shared<textx::Metamodel>(grammartext.str(),true,true,p);
    }

}