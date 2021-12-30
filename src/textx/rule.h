#pragma once

#include "textx/arpeggio.h"
#include "textx/grammar.h"
#include <unordered_map>
#include <string>
#include <variant>
#include <vector>
#include <unordered_set>
#include <memory>
#include <algorithm>

namespace textx::scoping {
    class RefResolver;
}

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

    struct AttributeInfo {
        AttributeCardinality cardinality = AttributeCardinality::scalar;
        std::optional<std::string> type = std::nullopt;
        std::vector<std::string> types={};
        bool is_text() const { return !type.has_value(); }
        void adjust_type(Metamodel& mm);
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
        std::unordered_map<std::string, AttributeInfo> attribute_info = {}; 
        std::unordered_set<std::string> m_tx_inh_by = {}; // for abstract rules 
        RuleType m_type = RuleType::illegal;
        std::unordered_map<std::string,std::string> m_tx_params = {};

        Rule(textx::Metamodel& mm, std::string_view name, textx::arpeggio::Match rule_params, const textx::arpeggio::Match& rule_body);
        void post_process_created_rule(textx::Metamodel& mm, std::string_view name, textx::arpeggio::Match rule_params, const textx::arpeggio::Match& rule_body, bool add_eof);
        textx::AttributeCardinality get_attribute_cardinality(const textx::arpeggio::Match& match, std::string name);
        bool adjust_tx_inh_by(textx::Metamodel& mm);
        void adjust_attr_types(textx::Metamodel& mm);
        textx::RuleType determine_rule_type(std::unordered_set<std::string> &recursion_stopper, const Metamodel& mm) const;

        friend Metamodel;
    public:
        Rule() = default;
        RuleType type() const { return m_type; }

        void add_attribute(std::string name, std::string type) {
            attribute_info[name].types.push_back(type);
        }

        void add_attribute(std::string name) {
            if (attribute_info.count(name)==0) {
                attribute_info[name] = {};
            }
        }

        const auto& tx_inh_by() const { return m_tx_inh_by; }
        const auto& tx_params() const { return m_tx_params; }
        const std::string& tx_params(std::string name) const { 
            auto p = m_tx_params.find(name);
            if (p==m_tx_params.end()) {
                throw std::runtime_error(std::string("cannot find param \"")+std::string(name)+"\"");
            }
            return p->second;
        }
        const std::string& tx_name() const { return name; }

        void add_tx_inh_by(std::string name) {
            //std::cout << this->name << " is inherited by " << name << "\n";
            m_tx_inh_by.insert(name);
        }

        std::optional<textx::arpeggio::Match> operator()(const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) const {
            return pattern(config, text, pos);
        }

        inline friend std::ostream& operator<<(std::ostream &o, const Rule& rule) {
            o << "Rule['"<< rule.name << "']";
            for(auto& [k,v]: rule.attribute_info) {
                o << " " << k << "=" << v;
            }
            return o;
        }

        const auto& get_attribute_info() const {
            return attribute_info;
        }

        AttributeInfo& operator[](std::string name)
        {
            auto p = attribute_info.find(name);
            if (p==attribute_info.end()) {
                throw std::runtime_error(std::string("cannot find attribute_info \"")+std::string(name)+"\"");
            }
            return p->second;
        }

        const AttributeInfo& operator[](std::string name) const
        {
            auto p = attribute_info.find(name);
            if (p==attribute_info.end()) {
                throw std::runtime_error(std::string("cannot find attribute_info \"")+std::string(name)+"\"");
            }
            return p->second;
        }
    };

}
