#include "textx/metamodel.h"
#include "textx/rule.h"
#include <cassert>
#include <unordered_set>

namespace textx {

    Metamodel::Metamodel(std::string_view grammar_text, bool is_main_grammar, bool include_basic_metamodel) {
        textx_grammar_parsetree.root = textx_grammar.parse_or_throw(grammar_text);        
        auto &root = textx_grammar_parsetree.root.value();

        assert(root.name && "unexpected: no textx model loaded!!");
        assert(root.name.value()=="rule://textx_model" && "unexpected: no textx model loaded!!");

        //TODO import and refs
                
        auto &rules = root.children[1];
        //std::cout << "children: " << rules.children.size() << "\n";

        bool first = true;
        try {
            for (auto&r : rules.children) {
                auto &rule_name = r.children[0].captured.value();
                //std::cout << "r: " << rule_name << "\n";
                auto &rule_params = r.children[1];
                auto &rule_body = r.children[3];
                auto rule_info = textx::parsetree::RuleInfo{r,rule_name};
                bool add_eof = first && is_main_grammar;
                auto new_rule = textx::createRuleFromTextxPattern(*this, rule_name, rule_params, rule_body, rule_info, add_eof);
                if (first) {
                    grammar.set_main_rule(rule_name);
                    first = false;
                }
                grammar.add_rule(rule_name, new_rule);
                textx_grammar_parsetree.rule_info.emplace(rule_name, rule_info);
            }

            if(include_basic_metamodel) {
                textx_grammar_parsetree.copy_rule_infos_from("", get_basic_metamodel().textx_grammar_parsetree);
            }

            textx_grammar_parsetree.finalize_rule_info();

            for (auto&[name,r] : textx_grammar_parsetree.rule_info) {
                if (!r.external_rule) {
                    auto &rule = grammar[name];
                    rule.m_type = r.rule_type;
                    rule.tx_inh_by = r.tx_inh_by;
                    for(auto& [name, info]: r.attribute_info) {
                        rule.attribute_info[name].type = info.type;
                        rule.attribute_info[name].cardinality = r.get_attribute_cardinality(name);
                    }
                }
            }
        }
        catch(textx::arpeggio::Exception &e) {
            std::ostringstream o;
            o << "textx grammar setup problem:\n";
            textx::arpeggio::print_error_position(o, grammar_text, e.pos);
            o << "\n" << e.what();
            throw std::runtime_error(o.str());
        }
    }

    Metamodel& Metamodel::get_basic_metamodel() {
        static Metamodel mm{R"(
            ID: /[^\d\W]\w*\b/;
            BOOL: /(True|true|False|false|0|1)\b/;
            INT: /[-+]?[0-9]+\b/;
            FLOAT: /[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?(?<=[\w\.])(?![\w\.])/;
            STRICTFLOAT: /[+-]?(((\d+\.(\d*)?|\.\d+)([eE][+-]?\d+)?)|((\d+)([eE][+-]?\d+)))(?<=[\w\.])(?![\w\.])/;
            STRING: /("(\"|[^"])*")|('(\'|[^'])*')/;
            NUMBER: STRICTFLOAT|INT;
            BASETYPE: NUMBER|FLOAT|BOOL|ID|STRING;
        )", false, false};
        return mm;
    }

    textx::arpeggio::Pattern Metamodel::ref(std::string name) {
        return [this,name](const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) -> std::optional<textx::arpeggio::Match>
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
        };

        //TODO handle referenced/included metamodels
        if (grammar.get_rules().find(name)==grammar.get_rules().end()) {
            std::cout << "?????? " << name << "\n";
            // if (this != &get_basic_metamodel()) {
            //     return get_basic_metamodel().ref(name);
            // }
        }
        return grammar.ref(name);
    }

    Rule& Metamodel::operator[](std::string name) {
        return grammar[name];
    }

    const Rule& Metamodel::operator[](std::string name) const {
        return grammar[name];
    }

    std::shared_ptr<textx::Model> Metamodel::model_from_str(std::string_view text) {
        auto parsetree = parsetree_from_str(text);
        auto ret=std::shared_ptr<textx::Model>{new textx::Model()}; // call private constructor (new)
        ret->init(text, *parsetree, shared_from_this());
        return ret;
    }

}