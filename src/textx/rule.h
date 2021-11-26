#pragma once

#include "arpeggio.h"
#include "grammar.h"
#include <unordered_map>
#include <string>
#include <variant>
#include <vector>
#include <unordered_set>
#include <algorithm>

namespace textx {

    enum class AttributeCardinality {scalar, list};
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

    struct AttributeInfo {
        AttributeCardinality cardinality = AttributeCardinality::scalar;
        std::vector<std::string> types;        
        std::optional<std::string> type = std::nullopt;
        bool is_text() const { return !type.has_value(); }
    };

    inline std::ostream& operator<<(std::ostream &o, const AttributeInfo& ai) {
        switch(ai.cardinality) {
            case AttributeCardinality::scalar: o << "scalar"; break;
            case AttributeCardinality::list: o << "list"; break;
            default: throw std::runtime_error("unknown AttributeInfo");
        }
        if (ai.is_text()) {
            o << "/text";
        }
        else {
            o << "/type(" << ai.type.value() << ")";
        }
        return o;
    }

    inline std::ostream& operator<<(std::ostream &o, const RuleType& rt) {
        switch(rt) {
            case RuleType::illegal: o << "illegal"; break;
            case RuleType::common: o << "common"; break;
            case RuleType::match: o << "match"; break;
            case RuleType::abstract: o << "abstract"; break;
            default: throw std::runtime_error("unknown RuleType");
        }
        return o;
    }

    class Metamodel;

    class Rule {
        textx::arpeggio::Pattern pattern;
        std::string name = "unnamed";
        std::unordered_map<std::string, AttributeInfo> attribute_info; 
        std::unordered_set<std::string> tx_inh_by; // for abstract rules 
        std::unordered_set<std::string> tx_bases;
        RuleType m_type = RuleType::illegal;
    public:
        RuleType type() const { return m_type; }
    
        std::optional<textx::arpeggio::Match> operator()(const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) {
            return pattern(config, text, pos);
        }

        inline friend std::ostream& operator<<(std::ostream &o, const Rule& rule) {
            o << "Rule['"<< rule.name << "']";
            for(auto& [k,v]: rule.attribute_info) {
                o << " " << k << "=" << v;
            }
            return o;
        }

        void add_tx_inh_by(std::string name) {
            tx_inh_by.insert(name);
        }
        void fix_tx_inh_by(textx::Metamodel& mm);
        void fix_attribute_types(const textx::Metamodel& mm);

        void add_attribute(std::string name, AttributeInfo base_info) {
            if (attribute_info.count(name)==0) {
                attribute_info[name] = base_info;
            }
            else {
                attribute_info[name].cardinality = AttributeCardinality::list;
                attribute_info[name].types.insert(
                    attribute_info[name].types.end(),
                    base_info.types.begin(),
                    base_info.types.end()
                );
            }
        }

        const auto& get_attribute_info() const {
            return attribute_info;
        }

        AttributeInfo& operator[](std::string name)
        {
            if (attribute_info.find(name)==attribute_info.end()) {
                throw std::runtime_error(std::string("cannot find attribute_info \"")+std::string(name)+"\"");
            }
            return attribute_info[name];
        }

        textx::RuleType determine_rule_type(std::unordered_set<std::string> &recursion_stopper, const textx::Metamodel& mm) const;

        void set_rule_type(RuleType t) {
            if (m_type!=RuleType::illegal) {
                throw std::runtime_error("illegal: you can only set the rule type once");
            }
            m_type = t;
        }

        friend Rule createRuleFromTextxPattern(textx::Metamodel& mm, std::string_view name, textx::arpeggio::Match rule_params, const textx::arpeggio::Match& rule_body, bool add_eof);
    };

    Rule createRuleFromTextxPattern(textx::Metamodel& mm, std::string_view name, textx::arpeggio::Match rule_params, const textx::arpeggio::Match& rule_body, bool add_eof);
}
