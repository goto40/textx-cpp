#pragma once
#include <cstdlib>
#include <vector>
#include <string>
#include <optional>

namespace textx
{
    namespace arpeggio
    {

        struct Match
        {
            size_t start, end;
            std::vector<Match> children = {};
        };

        auto str_match(std::string s)
        {
            return [=](std::string_view text, size_t pos) -> std::optional<Match>
            {
                if (text.substr(pos).starts_with(s))
                {
                    return Match{pos, pos + s.length()};
                }
                else
                {
                    return std::nullopt;
                }
            };
        }

    }
}