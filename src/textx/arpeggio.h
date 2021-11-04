#pragma once

#include <cstdlib>
#include <vector>
#include <string>
#include <optional>
#include <ostream>
#include <unordered_map>

namespace textx
{
    namespace arpeggio
    {

        enum class MatchType {
            str_match,
        };

        struct Match
        {
            size_t start, end;
            MatchType type;
            std::vector<Match> children = {};
            std::optional<std::string> name = std::nullopt;
            std::optional<std::string> captured = std::nullopt;

            static std::unordered_map<MatchType, std::string> type2str;
            friend std::ostream &operator<<(std::ostream &o, const Match &match)
            {
                o << "<" << type2str[match.type];                
                if (match.name.has_value())
                {
                    o << ":" << match.name.value();
                }
                o << ">(";
                for(auto &child: match.children) {
                    o << child;
                }
                o << ")";
                return o;
            }
        };

        template<class Pattern>
        auto named(std::string name, Pattern pattern)
        {
            return [=](std::string_view text, size_t pos) -> std::optional<Match>
            {
                auto match = pattern(text, pos);
                if (match.has_value())
                {
                    match.value().name = name;
                }
                return match;
            };
        }

        inline auto str_match(std::string s)
        {
            return [=](std::string_view text, size_t pos) -> std::optional<Match>
            {
                if (text.substr(pos).starts_with(s))
                {
                    return Match{pos, pos + s.length(), MatchType::str_match};
                }
                else
                {
                    return std::nullopt;
                }
            };
        }

    }
}