#pragma once
#include "arpeggio.h"
#include <unordered_map>
#include <string>

namespace textx
{
    class Grammar
    {
        textx::arpeggio::Pattern main = {};
        textx::arpeggio::Config config = {};
        textx::arpeggio::ParserState state = {""};
        bool ok = true;
        std::unordered_map<std::string, textx::arpeggio::Pattern> rules = {};

    public:
        Grammar() = default;
        Grammar(textx::arpeggio::Pattern text) : main{text} {}

        auto ref(std::string name)
        {
            return textx::arpeggio::rule(textx::arpeggio::named(name, [this,name](const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) -> std::optional<textx::arpeggio::Match>
            {
                if (rules.find(name)==rules.end()) {
                    throw std::runtime_error(std::string("cannot find ref(\"")+name+"\");");
                }
                std::cout << "ref(" << name << ")@" << pos <<"\n";
                auto res = rules.at(name)(config, text, pos);
                if (res) {
                    std::cout << "=> ref(" << name << ")@" << pos << " --> " << res.value().start << ".." << res.value().end << ", " << res.value() << "='" << textx::arpeggio::get_str(text.str(),res.value()) << "'\n";
                }
                return res;
            }));
        }

        void add_rule(std::string_view name, textx::arpeggio::Pattern p)
        {
            rules[std::string{name}] = p;
        }

        std::optional<textx::arpeggio::Match> parse(std::string_view text)
        {
            if (!main)
            {
                throw std::runtime_error("unexpected: no main rule defined in grammar");
            }
            state = textx::arpeggio::ParserState{text};
            auto res = main(config, state, {});
            ok = {res};
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

        void set_main_rule(textx::arpeggio::Pattern p)
        {
            main = p;
        }

        std::string get_last_error_string()
        {
            std::ostringstream s;
            auto pos = get_last_error_position();
            s << pos.text_position.line << ":" << pos.text_position.col << ":"
              << "expected " << pos;
            return s.str();
        }
    };

}