#pragma once
#include "textx/lang.h"
#include "textx/grammar.h"
#include "textx/rule.h"
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
        std::optional<textx::arpeggio::Match> grammar_root = {};
        //textx::parsetree::TextxGrammarParsetree textx_grammar_parsetree;
        static std::shared_ptr<Metamodel> get_basic_metamodel();
        std::unique_ptr<textx::scoping::RefResolver> default_resolver = std::make_unique<textx::scoping::PlainNameRefResolver>();
        std::unordered_map<std::string, std::unique_ptr<textx::scoping::RefResolver>> resolver = {};
        std::vector<std::shared_ptr<textx::Model>> builtin_models={};
        std::shared_ptr<textx::Workspace> default_workspace;
        std::vector<std::weak_ptr<textx::Metamodel>> imported_models={};
        std::unordered_map<std::string, std::weak_ptr<textx::Metamodel>> imported_models_by_name={};
        std::vector<std::weak_ptr<textx::Metamodel>> referenced_models={};
        std::unordered_map<std::string, std::weak_ptr<textx::Metamodel>> referenced_models_by_name={};
        std::string grammar_name="";
        std::unordered_set<std::string> all_types={};
        void adjust_tx_inh_by();
        void get_all_types(std::unordered_set<std::string> &res);

        public:
        Metamodel(std::string_view grammar, bool include_basic_metamodel=true, std::string filename="", std::shared_ptr<textx::Workspace> workspace=nullptr);
        std::shared_ptr<textx::Workspace> tx_default_workspace();
        bool is_instance(std::string special, std::string base) const;
        bool is_base_of(std::string base, std::string special) const { 
            //std::cout << "IS_BASE_OF\n";
            return is_instance(special, base);
        }
        Rule& operator[](std::string name);
        const Rule& operator[](std::string name) const;
        Rule& find_rule(std::string name, bool allow_referenced_mm);
        const Rule& find_rule(std::string name, bool allow_referenced_mm) const;
        bool has_rule(std::string name, bool allow_referenced_mm) const;
        std::string get_fqn_for_rule(std::string name) const;
        textx::arpeggio::Pattern ref(std::string name);
        std::string tx_grammar_name() {
            TEXTX_ASSERT(not grammar_name.empty());
            return grammar_name;
        }
        std::string tx_main_rule_name() {
            return grammar.get_main_rule_name();
        }

        auto begin() { return grammar.begin(); }
        auto end() { return grammar.end(); }
        auto begin() const { return grammar.begin(); }
        auto end() const { return grammar.end(); }
        size_t size() const { return grammar.size(); }

        std::shared_ptr<textx::Model> model_from_str(std::string_view text, std::string filename="", bool is_main_model=true, std::shared_ptr<textx::Workspace> workspace=nullptr);
        std::shared_ptr<textx::Model> model_from_file(std::filesystem::path p, bool is_main_model=true, std::shared_ptr<textx::Workspace> workspace=nullptr);

        const auto& tx_all_types() const { return all_types; }
        void add_builtin_model(std::shared_ptr<textx::Model> m) { builtin_models.push_back(std::move(m)); }
        void clear_builtin_models() { builtin_models.clear(); }

        bool has_non_default_resolver(std::string l) const {
            auto res = resolver.find(l);
            if (res!=resolver.end()) return true;
            for(auto weak_other_mm: imported_models) {
                auto other_mm = weak_other_mm.lock();
                TEXTX_ASSERT(other_mm != nullptr);
                if (other_mm->has_non_default_resolver(l)) {
                    return true;
                }
            }
            return false;
        }

        const textx::scoping::RefResolver& get_resolver(std::string l) const {
            auto res = resolver.find(l);
            if (res!=resolver.end()) return *res->second;
            for(auto weak_other_mm: imported_models) {
                auto other_mm = weak_other_mm.lock();
                TEXTX_ASSERT(other_mm != nullptr);
                if (other_mm->has_non_default_resolver(l)) {
                    return other_mm->get_resolver(l);
                }
            }
            throw std::runtime_error(std::string("unexpected error during lookup of resolver ")+l);
        }

        const textx::scoping::RefResolver& get_resolver(std::string rule_name, std::string attr_name) const {
            std::string lookup[] = {
                rule_name+"."+attr_name,
                std::string("*.")+attr_name,
                rule_name+".*",
                "*.*"
            };
            for (auto l: lookup) {
                if (has_non_default_resolver(l)) {
                    return get_resolver(l);
                }
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

    inline auto metamodel_from_str(std::string_view grammar, std::string filename="", std::shared_ptr<textx::Workspace> workspace=nullptr) {
        return std::make_shared<textx::Metamodel>(grammar, true, filename, workspace);
    }

    inline auto metamodel_from_file(std::filesystem::path p, std::shared_ptr<textx::Workspace> workspace=nullptr) {
        //std::cout << "load " << p << " to " << workspace << "\n";
        std::ifstream file(p);
        std::stringstream grammartext;
        grammartext << file.rdbuf();
        return std::make_shared<textx::Metamodel>(grammartext.str(),true ,p, workspace);
    }

}