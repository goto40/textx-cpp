#include "arpeggio.h"
#include <unordered_map>
#include <string>
#include <variant>
#include <vector>

namespace textx {

    struct ScalarAttribute {
        bool is_reference();
    };

    struct ListAttribute {
        bool is_reference();
    };

    class Rule {
        std::variant<CommonRule,AbstractRule,MatchRule> concrete_rule;
    public:
        Rule(CommonRule &&r) { concrete_rule = r; }
        Rule(AbstractRule &&r) { concrete_rule = r; }
        Rule(MatchRule &&r) { concrete_rule = r; }
        std::optional<textx::arpeggio::Match> operator()(const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, TextPosition pos) {
            std::visit([&](auto &r){
                return r(config, text, pos);
            }, concrete_rule);
        }
    };

    class CommonRule {
        std::unordered_map<std::string, ScalarAttribute> scalars;
        std::unordered_map<std::string, ListAttribute> lists;
            std::optional<textx::arpeggio::Match> operator()(const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, TextPosition pos) {
            //TODO
            return std::nullopt;
        }
   };
    class AbstractRule {
        std::vector<std::string> inner_rule_names;
        std::optional<textx::arpeggio::Match> operator()(const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, TextPosition pos) {
            //TODO
            return std::nullopt;
        }
    };
    class MatchRule {
        std::optional<textx::arpeggio::Match> operator()(const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, TextPosition pos) {
            //TODO
            return std::nullopt;
        }
    };


}