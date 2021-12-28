#pragma once

#include "textx/arpeggio.h"
#include "textx/grammar.h"
#include "textx/textx_grammar_parsetree.h"
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
        std::unordered_map<std::string, AttributeInfo> attribute_info = {}; 
        std::unordered_set<std::string> m_tx_bases = {};
        RuleType m_type = RuleType::illegal;
        std::unordered_map<std::string,std::string> m_tx_params = {};

        Rule(textx::Metamodel& mm, std::string_view name, textx::arpeggio::Match rule_params, const textx::arpeggio::Match& rule_body, textx::parsetree::RuleInfo& rule_info);
        void post_process_created_rule(textx::Metamodel& mm, std::string_view name, textx::arpeggio::Match rule_params, const textx::arpeggio::Match& rule_body, textx::parsetree::RuleInfo& rule_info, bool add_eof);

        friend Metamodel;
    public:
        Rule() = default;
        RuleType type() const { return m_type; }

        const auto& tx_bases() const { return m_tx_bases; }
        const auto& tx_params() const { return m_tx_params; }
        const std::string& tx_params(std::string name) const { 
            auto p = m_tx_params.find(name);
            if (p==m_tx_params.end()) {
                throw std::runtime_error(std::string("cannot find param \"")+std::string(name)+"\"");
            }
            return p->second;
        }
        const std::string& tx_name() const { return name; }

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
