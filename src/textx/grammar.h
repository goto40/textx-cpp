#pragma once
#include "arpeggio.h"
#include <unordered_map>
#include <string>
#include <concepts>

namespace textx
{
    template<std::convertible_to<textx::arpeggio::Pattern> R = textx::arpeggio::Pattern>
    class Grammar
    {
        std::string main_rule_name = "main";
        textx::arpeggio::Config config = {};
        textx::arpeggio::ParserState state = {""};
        bool ok = true;
        std::unordered_map<std::string, R> rules = {};

    public:
        Grammar() = default;
        Grammar(R r) { add_rule("main", r); }

        auto ref(std::string name)
        {
            return textx::arpeggio::rule(textx::arpeggio::named(name, [this,name](const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) -> std::optional<textx::arpeggio::Match>
            {
                if (rules.find(name)==rules.end()) {
                    throw std::runtime_error(std::string("cannot find ref(\"")+name+"\");");
                }
                return rules.at(name)(config, text, pos);
            }));
        }

        auto copy(std::string name)
        {
            return textx::arpeggio::rule(textx::arpeggio::named(name, [this,name](const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) -> std::optional<textx::arpeggio::Match>
            {
                if (rules.find(name)==rules.end()) {
                    throw std::runtime_error(std::string("cannot find ref(\"")+name+"\");");
                }
                auto res = rules.at(name)(config, text, pos);
                if (res.has_value()) {
                    return textx::arpeggio::Match{
                        res.value().start(),
                        res.value().end(),
                        textx::arpeggio::MatchType::custom,
                        {res.value()}
                    };
                }
                else {
                    return res;
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

        const auto& get_rules() const { return rules; }

        void add_rule(std::string_view name, R p)
        {
            if constexpr (std::is_same_v<textx::arpeggio::Pattern, R>) {
                rules.emplace(std::string{name}, textx::arpeggio::rule(textx::arpeggio::named(static_cast<std::string>(name), p)));
            }
            else {
                rules.emplace(std::string{name}, p);
            }
        }

        std::optional<textx::arpeggio::Match> parse(std::string_view text)
        {
            if (rules.count(main_rule_name)!=1)
            {
                throw std::runtime_error("unexpected: no main rule defined in grammar");
            }
            auto &main = rules[main_rule_name];
            state = textx::arpeggio::ParserState{text};
            auto res = main(config, state, {});
            ok = {res};
            return res;
        }

        std::optional<textx::arpeggio::Match> parse_or_throw(std::string_view text)
        {
            auto res = parse(text);
            if (!res) {
                throw std::runtime_error( get_last_error_string(text) );
            }
            return res;
        }

        textx::arpeggio::AnnotatedTextPosition get_last_error_position()
        {
            if (ok)
            {
                throw std::runtime_error("unexpected: called get_last_error_position() w/o error.");
            }
            return state.farthest_position;
        }

        void set_main_rule(std::string_view name="main")
        {
            main_rule_name = name;
        }

        std::string get_last_error_string(std::optional<std::string_view> text = std::nullopt)
        {
            std::ostringstream s;
            auto pos = get_last_error_position();
            if (text) {
                textx::arpeggio::print_error_position(s, text.value(), pos.text_position);
            }
            s << pos.text_position.line << ":" << pos.text_position.col << ":"
              << "expected\n" << pos;
            return s.str();
        }

        auto& get_config() {
            return config;
        }
    };

}