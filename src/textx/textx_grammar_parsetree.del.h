#pragma once

#include "textx/arpeggio.h"
#include "textx/assert.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace textx {
    class Metamodel;

    enum class AttributeCardinality {scalar, list, boolean};
    enum class RuleType {
        illegal,
        common,    /// Common rules are rules that contain at least one assignment, i.e., they have attributes defined.  
        abstract,  /// Abstract rules are rules that have no assignments and reference at least one abstract or common rule. They are usually given as an ordered choice of other rules and they are used to generalize other rules. 
        match      /// Match rules are rules that have no assignments either direct or indirect, i.e. all referenced rules are match rules too. 
    };
    /* Details:
     * 
     * Abstract rules:
     *  - Abstract rules may reference match rules and base types.
     *  - Abstract rules can be a complex mix of rule references and match expressions as long as there is at least one abstract or common reference.
     *  - A rule with a single reference to an abstract or common rule is also abstract
     *  - More details:
     *    - If all rule references in a single alternative are match rules the result will be a concatenation of all match rule results,
     *    - If there is a common rule reference than it would be the result and all surrounding match rules are used only for parsing.
     *    - If there are multiple common rules than the first will be used as a result and the rest only for parsing
     *  
     * match rules: 
     *  - These rules can be used in match references only (i.e., you can't link to these rules as they don't exists as objects)
     */

    AttributeCardinality get_multiplicity(textx::arpeggio::Match &match);
    bool is_assignment_to_attribute(textx::arpeggio::Match &match, std::string name);
}
namespace textx::scoping {
    class RefResolver;    
}

namespace textx::parsetree {

    struct AttributeInfo {
        std::vector<std::string> types={};
        std::optional<std::string> type = std::nullopt;
    };

    class TextxGrammarParsetree;
    struct RuleInfo {
        textx::arpeggio::Match &match;
        std::string name;
        std::unordered_map<std::string, AttributeInfo> attribute_info;
        std::unordered_set<std::string> tx_inh_by; // for abstract rules 
        std::unordered_set<std::string> tx_bases;
        textx::RuleType rule_type=textx::RuleType::illegal;
        bool external_rule=false;

        RuleInfo(textx::arpeggio::Match &match, std::string name) :match{match}, name(name) {}
        RuleInfo(const RuleInfo&) = default;

        void add_tx_inh_by(std::string name) {
            tx_inh_by.insert(name);
        }
        void fix_tx_inh_by(TextxGrammarParsetree& p);
        void fix_attribute_types(const TextxGrammarParsetree& p);

        void add_attribute(std::string name, std::string type) {
            attribute_info[name].types.push_back(type);
        }
        void add_attribute(std::string name) {
            if (attribute_info.count(name)==0) {
                attribute_info[name] = {};
            }
        }

        const std::string& tx_name() const { return name; }

        textx::AttributeCardinality get_attribute_cardinality(std::string name);
        textx::RuleType determine_rule_type(std::unordered_set<std::string> &recursion_stopper, const TextxGrammarParsetree& p) const;
    };

    struct TextxGrammarParsetree {
        std::optional<textx::arpeggio::Match> root;
        std::unordered_map<std::string, RuleInfo> rule_info;
        void finalize_rule_info();

        RuleInfo& operator[](std::string name)
        {
            auto f=rule_info.find(name);
            if (f==rule_info.end()) {
                std::ostringstream o;
                o << "cannot find rule_info \"" << name << "\"";
                throw std::runtime_error(o.str());
            }
            return f->second;
        }

        const RuleInfo& operator[](std::string name) const
        {
            auto f=rule_info.find(name);
            if (f==rule_info.end()) {
                std::ostringstream o;
                o << "cannot find rule_info \"" << name << "\"";
                throw std::runtime_error(o.str());
            }
            return f->second;
        }

        void copy_rule_infos_from(std::string grammar_name, const TextxGrammarParsetree& other) {
            for(const auto &[name, ri]: other.rule_info) {
                std::string new_name = name;
                if (grammar_name.size()>0) {
                    new_name = grammar_name+"."+new_name;
                }
                if (rule_info.count(name)==0) { // onyl insert if same rule does not exist here
                    auto res = rule_info.emplace(name,ri);
                    TEXTX_ASSERT(res.second, "insertion must be ok")
                    auto &new_ri = res.first->second;
                    new_ri.external_rule = true;
                    if (name.size()>0) {
                        for(auto &[aname, ai]: new_ri.attribute_info) {
                            for(auto &t: ai.types) {
                                if (t.find(".")==-1) {
                                    t = name+"."+t;
                                }
                            }
                            if (ai.type.has_value()) {
                                auto &t = ai.type.value();
                                if (t.find(".")==-1) {
                                    t = name+"."+t;
                                }
                            }
                        }
                    }
                }
            }
        }
    };

}
