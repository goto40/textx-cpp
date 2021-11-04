#pragma once

#include <cstdlib>
#include <vector>
#include <string>
#include <optional>
#include <ostream>
#include <unordered_map>
#include <regex>

namespace textx
{
    namespace arpeggio
    {

        enum class MatchType {
            str_match,
            regex_match,
        };

        struct Match
        {
            size_t start, end;
            MatchType type;
            std::vector<Match> children = {};
            std::optional<std::string> name = std::nullopt;
            std::optional<std::string> captured = std::nullopt;

            static std::unordered_map<MatchType, std::string> type2str;
            static std::unordered_map<MatchType, bool> is_terminal;
            friend std::ostream &operator<<(std::ostream &o, const Match &match)
            {
                o << "<" << type2str.at(match.type);                
                if (match.name.has_value())
                {
                    o << ":" << match.name.value();
                }
                if (match.captured.has_value())
                {
                    o << " captured=" << match.captured.value();
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

        inline std::string_view get_str(std::string_view text, Match match) {
            return text.substr(match.start, match.end-match.start);
        }

        inline bool is_terminal(const Match& m) {
            return Match::is_terminal.at(m.type);
        }

        template<class Pattern>
        auto capture(Pattern pattern)
        {
            return [=](std::string_view text, size_t pos) -> std::optional<Match>
            {
                auto match = pattern(text, pos);
                if (match.has_value())
                {
                    match.value().captured = get_str(text, match.value());
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

        inline auto regex_match(std::string s)
        {
            return [r=std::regex{std::string("(")+s+").*"}](std::string_view text, size_t pos) -> std::optional<Match>
            {
                std::match_results<std::string_view::const_iterator> smatch;
                if (std::regex_match(text.begin()+pos, text.end(), smatch, r))  
                {
                    return Match{pos, pos + smatch[1].length(), MatchType::regex_match};
                }
                else
                {
                    return std::nullopt;
                }
            };
        }

    }
}