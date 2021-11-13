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
                // std::cout << "> ref(" << name << ")@" << pos <<"\n";
                auto res = rules.at(name)(config, text, pos);
                // if (res) {
                //     std::cout << "< => ref(" << name << ")@" << pos << " --> " << res.value().start << ".." << res.value().end << ", " << res.value() << "='" << textx::arpeggio::get_str(text.str(),res.value()) << "'\n";
                // }
                return res;
            }));
        }

        void add_rule(std::string_view name, R p)
        {
            if constexpr (std::is_same_v<textx::arpeggio::Pattern, R>) {
                rules[std::string{name}] = textx::arpeggio::rule(p);
            }
            else {
                rules[std::string{name}] = p;
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
                const size_t p = pos.text_position.pos;
                const size_t n = std::min<size_t>(p, 30);
                const size_t m = std::min<size_t>(text.value().length()-p, 30);
                s << "..." << text.value().substr(pos.text_position.pos-n,n);
                s << "*\n" << text.value().substr(pos.text_position.pos,m);
                s << "...\n";
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