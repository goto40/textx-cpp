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
        bool default_skipws = true;

    public:
        Grammar() = default;
        Grammar(R r) { add_rule("main", r); }

        void set_default_skipws(bool s) { default_skipws=s; }

        auto ref(std::string name)
        {
            std::string rname = std::string("rule://")+name;
            return textx::arpeggio::rule(textx::arpeggio::named(rname, [this,name](const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) -> std::optional<textx::arpeggio::Match>
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
            return textx::arpeggio::rule(textx::arpeggio::named(rname, [this,name](const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) -> std::optional<textx::arpeggio::Match>
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

        std::optional<textx::arpeggio::Match> parse(std::string_view text)
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
            state = textx::arpeggio::ParserState{text};
            auto res = main(config, state, {});
            ok = res.has_value();
            if (ok) {
                return res.value().children[0];
            }
            else {
                return std::nullopt;
            }
        }

        std::optional<textx::arpeggio::Match> parse_or_throw(std::string_view text)
        {
            auto res = parse(text);
            if (!res) {
                textx::arpeggio::raise( state.farthest_position.text_position, get_last_error_string(text) );
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
            main_rule_name = std::string(name);
        }
        std::string get_main_rule_name()
        {
            return main_rule_name;
        }

        std::string get_last_error_string(std::optional<std::string_view> text = std::nullopt)
        {
            std::ostringstream s;
            auto pos = get_last_error_position();
            // if (text) {
            //     textx::arpeggio::print_error_position(s, text.value(), pos.text_position);
            // }
            s << pos.text_position.line << ":" << pos.text_position.col << ":"
              << "expected\n" << pos;
            return s.str();
        }

        auto& get_config() {
            return config;
        }
    };

}