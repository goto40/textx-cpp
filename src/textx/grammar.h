#pragma once
#include "arpeggio.h"
#include "textx/assert.h"
#include <unordered_map>
#include <string>
#include <concepts>
#include <utility>

namespace textx
{
    template<std::convertible_to<textx::arpeggio::Pattern> R = textx::arpeggio::Pattern>
    class Grammar
    {
        std::string main_rule_name = "main";
        textx::arpeggio::Config config = {};
        std::unordered_map<std::string, R> rules = {};
        bool default_skipws = true;

    public:
        Grammar() = default;
        Grammar(R r) { add_rule("main", r); }

        void set_default_skipws(bool s) { default_skipws=s; }

        auto ref(std::string name)
        {
            std::string rname = std::string("rule://")+name;
            return textx::arpeggio::rule(textx::arpeggio::named(rname, [this,name](const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) -> arpeggio::ParserResult
            {
                if (rules.find(name)==rules.end()) {
                    throw std::runtime_error(std::string("cannot find ref(\"")+name+"\");");
                }
                return rules.at(name)(config, text, pos);
            }));
        }

        auto copy(std::string name)
        {
            std::string rname = std::string("rule://")+name;
            return textx::arpeggio::rule(textx::arpeggio::named(rname, [this,name](const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) -> arpeggio::ParserResult
            {
                if (rules.find(name)==rules.end()) {
                    throw std::runtime_error(std::string("cannot find ref(\"")+name+"\");");
                }
                auto parseResult = rules.at(name)(config, text, pos);
                if (parseResult.ok()) {
                    return textx::arpeggio::Match{
                        parseResult.value().start(),
                        parseResult.value().end(),
                        textx::arpeggio::MatchType::custom,
                        {parseResult.value()}
                    };
                }
                else {
                    return parseResult;
                }
            }));
        }

        R& operator[](std::string name)
        {
            if (rules.find(name)==rules.end()) {
                throw std::runtime_error(std::string("cannot find rule \"")+std::string(name)+"\"");
            }
            return rules[name];
        }

        const R& operator[](std::string name) const
        {
            auto f=rules.find(name);
            if (f==rules.end()) {
                throw std::runtime_error(std::string("cannot find rule \"")+std::string(name)+"\"");
            }
            return f->second;
        }

        const auto& get_rules() const { return rules; }

        auto begin() { return rules.begin(); }
        auto begin() const { return rules.begin(); }
        auto end() { return rules.end(); }
        auto end() const { return rules.end(); }
        size_t size() const { return rules.size(); }

        void add_rule(std::string_view name, R p)
        {
            //std::cout << "ADD RULE " << name << "\n";
            std::ostringstream n;
            n << "rule://" << name;
            if constexpr (std::is_same_v<textx::arpeggio::Pattern, R>) {
                rules.emplace(std::string{name}, textx::arpeggio::rule(textx::arpeggio::named(n.str(), p)));
            }
            else {
                rules.emplace(std::string{name}, p);
            }
        }

        std::pair<arpeggio::ParserResult, arpeggio::ParserState> parse(std::string_view text)
        {
            if (rules.count(main_rule_name)!=1)
            {
                for (auto [k,v]: rules) {
                    std::cout << k << "\n";
                }
                throw std::runtime_error(std::string("unexpected: no main rule found in grammar; name=")+main_rule_name);
            }
            auto main = textx::arpeggio::sequence({
                rules[main_rule_name],
                textx::arpeggio::end_of_file()
            });
            if (default_skipws==false) {
                main = textx::arpeggio::noskipws(main);
            }
            else {
                main = textx::arpeggio::skipws(main);
            }
            auto state = textx::arpeggio::ParserState{text, "(unnamed grammar)"};
            auto parseResult = main(config, state, {});
            if (parseResult.ok()) {
                parseResult.update_match(parseResult.value().children[0]);
            }
            else if (parseResult.value().children.size()>0) {
                parseResult.update_match(parseResult.value().children[0]);
            }
            return {parseResult, state};
        }

        arpeggio::ParserResult parse_or_throw(std::string_view text)
        {
            auto [res, state] = parse(text);
            if (!res) {
                textx::arpeggio::raise( state.farthest_position.text_position, state.get_last_error_string(text) );
            }
            return res;
        }

        void set_main_rule(std::string_view name="main")
        {
            main_rule_name = std::string(name);
        }
        std::string get_main_rule_name()
        {
            return main_rule_name;
        }

        auto& get_config() {
            return config;
        }
    };

}