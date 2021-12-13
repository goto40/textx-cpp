#include "textx/metamodel.h"
#include "textx/rule.h"
#include <cassert>
#include <unordered_set>

namespace textx {

    Metamodel::Metamodel(std::string_view grammar_text, bool is_main_grammar, bool include_basic_metamodel, std::string filename) {
        try {
            textx_grammar_parsetree.root = textx_grammar.parse_or_throw(grammar_text);
            auto &root = textx_grammar_parsetree.root.value();

            assert(root.name && "unexpected: no textx model loaded!!");
            assert(root.name.value()=="rule://textx_model" && "unexpected: no textx model loaded!!");

            //TODO import and refs
                    
            auto &rules = root.children[1];
            //std::cout << "children: " << rules.children.size() << "\n";

            bool first = true;
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
                    rule.m_tx_inh_by = r.tx_inh_by;
                    rule.m_tx_bases = r.tx_bases;
                    for(auto& [name, info]: r.attribute_info) {
                        rule.attribute_info[name].type = info.type;
                        rule.attribute_info[name].cardinality = r.get_attribute_cardinality(name);
                    }
                }
            }

            // comments
            if (has_rule("Comment")) {
                grammar.get_config().skip_text = textx::arpeggio::skip_text_functions::combine({
                    textx::arpeggio::skip_text_functions::skipws(),
                    textx::arpeggio::skip_text_functions::skip_pattern(grammar["Comment"])
                });
            }
        }
        catch(textx::arpeggio::Exception &e) {
            std::ostringstream o;
            o << filename << ":" << e.pos << ": textx grammar setup problem:\n";
            textx::arpeggio::print_error_position(o, grammar_text, e.pos);
            o << "\n" << e.what();
            e.error = o.str();
            throw e;
        }
        catch(std::exception &e) {
            throw std::runtime_error(filename+": "+e.what());
        }
    }

    bool Metamodel::has_rule(std::string name) const {
        return grammar.get_rules().count(name)>0;
    }

    Metamodel& Metamodel::get_basic_metamodel() {
        static Metamodel mm{R"(
            ID: /[^\d\W]\w*\b/;
            BOOL: /(True|true|False|false|0|1)\b/;
            INT: /[-+]?[0-9]+\b/;
            FLOAT: /[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?(?<=[\w\.])(?![\w\.])/;
            STRICTFLOAT: /[+-]?(((\d+\.(\d*)?|\.\d+)([eE][+-]?\d+)?)|((\d+)([eE][+-]?\d+)))(?<=[\w\.])(?![\w\.])/;
            STRING: /("(\\"|[^"])*")|('(\\'|[^'])*')/;
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
        // TOOD handle dot separated grammar names (grammar.rule)
        if (grammar.get_rules().count(name)>0) {
            return grammar[name];
        }
        else {
            return get_basic_metamodel()[name];
        }
    }

    const Rule& Metamodel::operator[](std::string name) const {
        // TOOD handle dot separated grammar names (grammar.rule)
        if (grammar.get_rules().count(name)>0) {
            return grammar[name];
        }
        else {
            return get_basic_metamodel()[name];
        }
    }

    std::shared_ptr<textx::Model> Metamodel::model_from_str(std::string_view text, std::string filename, bool is_main_model) {
        try {
            auto parsetree = parsetree_from_str(text);
            auto ret=std::shared_ptr<textx::Model>{new textx::Model()}; // call private constructor (new)
            //std::cout << parsetree.value() << "\n";
            ret->init(text, *parsetree, shared_from_this());

            textx::object::traverse(ret->val(), [&, this](textx::object::Value& v) {
                if (v.is_pure_obj() && v.obj()->has_attr("importURI")) {
                    TEXTX_ASSERT(v["importURI"].is_str(), "importURI must be a string");
                    auto filename = v["importURI"].str();
                    auto m = model_from_file(filename, false); // unresolved refs
                    ret->add_imported_model(m);
                }
            });
 
            for (auto& builtin_model: builtin_models) {
                ret->add_imported_model(builtin_model);
            }

            if (is_main_model) {
                // TODO recursive resolution
                if(ret->resolve_references()>0) {
                    std::stringstream error_text;
                    textx::arpeggio::TextPosition pos;
                    textx::object::traverse(ret->val(),[&](textx::object::Value& v) {
                        if (v.is_ref() && !v.ref().obj.lock()) {
                            error_text << "ref '" << v.ref().name << "' not found at " << v.pos << ";\n";
                            pos = v.pos;
                        }
                    });
                    //std::cout << ret->val() << "\n";
                    textx::arpeggio::raise(pos, error_text.str());
                }
            }
            return ret;
        }
        catch(textx::arpeggio::Exception &e) {
            e.filename = filename;
            e.error = filename + ":" + std::to_string(e.pos.line) +":" + std::to_string(e.pos.col) + ": " + e.error;
            throw e;
        }
        catch(std::exception &e) {
            throw std::runtime_error(filename+": "+e.what());
        }
    }

    std::shared_ptr<textx::Model> Metamodel::model_from_file(std::filesystem::path p, bool is_main_model) {
        auto abspath = std::filesystem::canonical(p);
        if (known_models.count(abspath.string())) {
            return known_models[abspath.string()]; // cached model
        }
        std::ifstream file(abspath);
        std::stringstream modeltext;
        modeltext << file.rdbuf();
        auto m = model_from_str(modeltext.str(), abspath.string(), is_main_model);
        known_models[abspath.string()] = m;
        return m;
    }

    bool Metamodel::is_base_of(std::string t1, std::string t2) const {
        //std::cout << "is_base_of " << t1 << " " << t2 << "?\n";
        TEXTX_ASSERT(has_rule(t1));
        TEXTX_ASSERT(has_rule(t2));
        if (t1==t2) return true;
        else if (operator[](t2).tx_bases().size()>0) {
            for (auto b: operator[](t2).tx_bases()) {
                auto res = is_base_of(t1,b);
                if (res) return res;
            }
        }
        return false;
    }
}