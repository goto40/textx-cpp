#pragma once

#include <cstdlib>
#include <vector>
#include <string>
#include <optional>
#include <iostream>
#include <unordered_map>
#include <regex>

namespace textx
{
    namespace arpeggio
    {

        enum class MatchType {
            str_match,
            regex_match,
            sequence,
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

        using SkipTextFun = std::function<size_t(std::string_view text, size_t pos)>;

        namespace skip_text_functions {
            inline auto nothing() {
                return [](std::string_view, size_t pos)->size_t {
                    return pos;
                };
            }            
            inline auto skipws() {
                return [=](std::string_view text, size_t pos)->size_t {
                    while(pos<text.length() && std::isspace(text[pos])) {
                        pos++;
                    }
                    return pos;
                };
            }            
            inline auto combine(std::initializer_list<SkipTextFun> ps) {
                return [=](std::string_view text, size_t pos)->size_t {
                    size_t pos0 = pos;
                    do {
                        pos0 = pos;
                        for (auto p: ps) {
                            pos = p(text,pos);
                        }
                    } while(pos0!=pos);
                    return pos;
                };
            }            
        }

        struct Config {
            SkipTextFun skip_text = skip_text_functions::skipws();
        };

        using Pattern = std::function<std::optional<Match>(const Config &config, std::string_view text, size_t pos)>;

        inline std::string_view get_str(std::string_view text, Match match) {
            return text.substr(match.start, match.end-match.start);
        }

        inline bool is_terminal(const Match& m) {
            return Match::is_terminal.at(m.type);
        }

        inline auto named(std::string name, Pattern pattern)
        {
            return [=](const Config &config, std::string_view text, size_t pos) -> std::optional<Match>
            {
                auto match = pattern(config, text, pos);
                if (match.has_value())
                {
                    match.value().name = name;
                }
                return match;
            };
        }

        inline auto capture(Pattern pattern)
        {
            return [=](const Config &config, std::string_view text, size_t pos) -> std::optional<Match>
            {
                auto match = pattern(config, text, pos);
                if (match.has_value())
                {
                    match.value().captured = get_str(text, match.value());
                }
                return match;
            };
        }

        inline auto str_match(std::string s)
        {
            return [=](const Config &config, std::string_view text, size_t pos) -> std::optional<Match>
            {
                pos = config.skip_text(text, pos);
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
            return [r=std::regex{std::string("(")+s+").*"}](const Config &config, std::string_view text, size_t pos) -> std::optional<Match>
            {
                pos = config.skip_text(text, pos);
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

        inline auto sequence(std::vector<Pattern> patterns)
        {
            return [=](const Config &config, std::string_view text, size_t pos) -> std::optional<Match>
            {
                Match match{pos, pos, MatchType::sequence};
                for (auto pattern: patterns) {
                    auto sub_match = pattern(config, text, pos);
                    if (sub_match)
                    {
                        pos = sub_match.value().end;
                        match.end = pos;
                        match.children.push_back(sub_match.value());                         
                    }
                    else {
                        return std::nullopt;
                    }
                }
                return match;
            };
        }

        inline auto ordered_choice(std::vector<Pattern> patterns)
        {
            return [=](const Config &config, std::string_view text, size_t pos) -> std::optional<Match>
            {
                for (auto pattern: patterns) {
                    auto match = pattern(config, text, pos);
                    if (match)
                    {
                        return match;
                    }
                }
                return std::nullopt;
            };
        }

    }
}