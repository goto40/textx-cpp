#pragma once

#include "arpeggio.h"
#include "grammar.h"
#include <unordered_map>
#include <string>
#include <variant>
#include <vector>

namespace textx {

    enum class AttributeCardinality {scalar, list};
    struct AttributeInfo {
        AttributeCardinality cardinality = AttributeCardinality::scalar;
        std::optional<std::string> type = std::nullopt;
        bool is_terminal() const { return !type.has_value(); }
    };
    inline std::ostream& operator<<(std::ostream &o, const AttributeInfo& ai) {
        switch(ai.cardinality) {
            case AttributeCardinality::scalar: o << "scalar"; break;
            case AttributeCardinality::list: o << "list"; break;
            default: throw std::runtime_error("unknown AttributeInfo");
        }
        if (ai.is_terminal()) {
            o << "/terminal";
        }
        else {
            o << "/type(" << ai.type.value() << ")";
        }
        return o;
    }

    struct Rule {
        textx::arpeggio::Pattern pattern;
        std::string name = "unnamed";
        std::unordered_map<std::string, AttributeInfo> attribute_info; 

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

        void add_attribute(std::string name, AttributeInfo base_info) {
            if (attribute_info.count(name)==0) {
                attribute_info[name] = base_info;
            }
            else {
                // TODO determine common type here
                base_info.cardinality = AttributeCardinality::list;
                attribute_info[name] = base_info;
            }
        }

        AttributeInfo& operator[](std::string name)
        {
            if (attribute_info.find(name)==attribute_info.end()) {
                throw std::runtime_error(std::string("cannot find attribute_info \"")+std::string(name)+"\"");
            }
            return attribute_info[name];
        }

    };

    class Metamodel;
    Rule createRuleFromTextxPattern(textx::Metamodel& mm, std::string_view name, textx::arpeggio::Match rule_params, const textx::arpeggio::Match& rule_body, bool add_eof);
}
