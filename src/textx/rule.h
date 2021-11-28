#pragma once

#include "arpeggio.h"
#include "grammar.h"
#include "parsetree.h"
#include <unordered_map>
#include <string>
#include <variant>
#include <vector>
#include <unordered_set>
#include <algorithm>

namespace textx {

    struct AttributeInfo {
        AttributeCardinality cardinality = AttributeCardinality::scalar;
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
        friend Metamodel;

        RuleType type() const { return m_type; }
    
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
            if (attribute_info.find(name)==attribute_info.end()) {
                throw std::runtime_error(std::string("cannot find attribute_info \"")+std::string(name)+"\"");
            }
            return attribute_info[name];
        }

        friend Rule createRuleFromTextxPattern(textx::Metamodel& mm, std::string_view name, textx::arpeggio::Match rule_params, const textx::arpeggio::Match& rule_body, textx::parsetree::RuleInfo& rule_info, bool add_eof);
    };

    Rule createRuleFromTextxPattern(textx::Metamodel& mm, std::string_view name, textx::arpeggio::Match rule_params, const textx::arpeggio::Match& rule_body, textx::parsetree::RuleInfo& rule_info, bool add_eof);
}
