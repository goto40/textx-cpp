#pragma once

#include <cstdlib>
#include <vector>
#include <string>
#include <optional>
#include <iostream>
#include <unordered_map>
#include <regex>
#include <tuple>
#include <compare>

namespace textx
{
    namespace arpeggio
    {

        enum class MatchType
        {
            undefined,
            end_of_file,
            str_match,
            regex_match,
            sequence,
            ordered_choice,
            negative_lookahead,
            positive_lookahead,
            one_or_more,
            zero_or_more,
            optional,
        };
        struct TextPosition
        {
            size_t pos = 0, line = 1, col = 1;
            auto operator<=>(const TextPosition &) const noexcept = default;
            operator size_t() { return pos; }

            void inc(std::string_view text, size_t amount = 1)
            {
                for (size_t i = 0; i < amount; i++)
                {
                    if (text[pos] == '\n')
                    {
                        col = 1;
                        line++;
                    }
                    else
                    {
                        col++;
                    }
                    pos++;
                }
            }
            TextPosition add(std::string_view text, size_t amount)
            {
                TextPosition me = *this;
                me.inc(text, amount);
                return me;
            }

            friend std::ostream &operator<<(std::ostream &o, const TextPosition &pos)
            {
                o << pos.line << ":" << pos.col;
                return o;
            }
        };

        struct Match
        {
            TextPosition start, end;
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
                for (auto &child : match.children)
                {
                    o << child;
                }
                o << ")";
                return o;
            }
        };

        struct AnnotatedTextPosition
        {
            TextPosition text_position = {};
            std::vector<std::tuple<MatchType, std::string>> info;
            auto operator<=>(const AnnotatedTextPosition &b) const noexcept { return text_position <=> b.text_position; }
            operator TextPosition() { return text_position; }

            friend std::ostream &operator<<(std::ostream &o, const AnnotatedTextPosition &pos)
            {
                for (auto &i : pos.info)
                {
                    o << "(" << Match::type2str.at(std::get<MatchType>(i)) << "," << std::get<std::string>(i) << ")";
                }
                return o;
            }
        };

        class ParserState
        {
            std::string_view source;
            static size_t cache_reset_indicator_source;
            size_t cache_reset_indicator = {cache_reset_indicator_source++};

        public:
            size_t cache_hits = {0};
            size_t cache_misses = {0};
            AnnotatedTextPosition farthest_position = {};

            size_t get_cache_reset_indicator() { return cache_reset_indicator; }
            ParserState(std::string_view s) : source(s) {}
            operator std::string_view() { return source; }
            size_t length() { return source.length(); }
            char operator[](size_t p) { return source[p]; }
            void update_farthest_position(TextPosition pos, MatchType type, std::string_view info)
            {
                if (pos > farthest_position)
                {
                    farthest_position = AnnotatedTextPosition{
                        .text_position = pos,
                        .info = {{type, std::string{info}}}};
                }
                else if (pos == farthest_position)
                {
                    farthest_position.info.push_back({type, std::string{info}});
                }
            }
            std::string_view str() { return source; }
        };

        using SkipTextFun = std::function<TextPosition(ParserState &text, TextPosition pos)>;

        namespace skip_text_functions
        {
            inline auto nothing()
            {
                return [](ParserState &, TextPosition pos) -> TextPosition
                {
                    return pos;
                };
            }
            inline auto skipws()
            {
                return [=](ParserState &text, TextPosition pos) -> TextPosition
                {
                    while (pos < text.length() && std::isspace(text[pos]))
                    {
                        pos.inc(text);
                    }
                    return pos;
                };
            }
            inline auto combine(std::vector<SkipTextFun> ps)
            {
                return [=](ParserState &text, TextPosition pos) -> TextPosition
                {
                    size_t pos0 = pos;
                    do
                    {
                        pos0 = pos;
                        for (auto p : ps)
                        {
                            pos = p(text, pos);
                        }
                    } while (pos0 != pos);
                    return pos;
                };
            }
        }

        struct Config
        {
            SkipTextFun skip_text = skip_text_functions::skipws();
        };

        using Pattern = std::function<std::optional<Match>(const Config &config, ParserState &text, TextPosition pos)>;

        inline std::string_view get_str(std::string_view text, Match match)
        {
            return text.substr(match.start, match.end - match.start);
        }

        inline bool is_terminal(const Match &m)
        {
            return Match::is_terminal.at(m.type);
        }

        // decorator
        /**
         * This function is a rule wrapper requried for each rule.
         * It manages memoization and basic checks.
         *
         * If you omit the rule-call in your high-level grammar you loose some efficiency.
         */
        inline auto rule(Pattern pattern)
        {
            return [=, cache = std::unordered_map<size_t, std::optional<Match>>{}, chached_state = static_cast<size_t>(0)](const Config &config, ParserState &text, TextPosition pos) mutable -> std::optional<Match>
            {
                // basic checks:
                if (pos > text.length())
                {
                    throw std::runtime_error("unexpected: pos>text.length()");
                }

                // memoization:
                if (chached_state != text.get_cache_reset_indicator())
                {
                    chached_state = text.get_cache_reset_indicator();
                    cache = {};
                }
                auto res = cache.find(pos);
                if (res != cache.end() && res->second.has_value())
                { // recursion breaker (2nd part)
                    text.cache_hits++;
                    return *(res->second);
                }
                else
                {
                    text.cache_misses++;
                    cache[pos] = std::nullopt; // recursion breaker (1st part)

                    auto match = pattern(config, text, pos);

                    if (match)
                    {
                        if (match.value().type == MatchType::undefined)
                        {
                            std::ostringstream s;
                            s << "unexpected, found undefined result = " << match.value();
                            throw std::runtime_error(s.str());
                        }
                    }
                    cache[pos] = match;
                    return match;
                }
            };
        }

        // decorator
        inline auto named(std::string name, Pattern pattern)
        {
            return [=](const Config &config, ParserState &text, TextPosition pos) -> std::optional<Match>
            {
                auto match = pattern(config, text, pos);
                if (match.has_value())
                {
                    match.value().name = name;
                }
                else
                {
                    text.update_farthest_position(pos,MatchType::str_match,std::string("rule-name="+name));
                }
                return match; };
        }

        // decorator
        inline auto capture(Pattern pattern)
        {
            return [=](const Config &config, ParserState &text, TextPosition pos) -> std::optional<Match>
            {
                auto match = pattern(config, text, pos);
                if (match.has_value())
                {
                    match.value().captured = get_str(text, match.value());
                }
                return match; };
        }

        inline auto optional(Pattern pattern)
        {
            return rule([=](const Config &config, ParserState &text, TextPosition pos) -> std::optional<Match>
                        {
                auto match = pattern(config, text, pos);
                if (match.has_value())
                {
                    return Match{.start=match.value().start, .end=match.value().end, .type=MatchType::optional, .children={match.value()}};
                }
                else {
                    return Match{.start=pos, .end=pos, .type=MatchType::optional, .children={}};
                } });
        }

        inline auto str_match(std::string s)
        {
            return rule([=](const Config &config, ParserState &text, TextPosition pos) -> std::optional<Match>
                        {
                pos = config.skip_text(text, pos);
                if (text.str().substr(pos).starts_with(s))
                {
                    return Match{pos, pos.add(text, s.length()), MatchType::str_match};
                }
                else
                {
                    text.update_farthest_position(pos,MatchType::str_match,s);
                    return std::nullopt;
                } });
        }

        inline auto regex_match(std::string s)
        {
            return rule([=, r = std::regex{std::string("^") + s}](const Config &config, ParserState &text, TextPosition pos) -> std::optional<Match>
                        {
                pos = config.skip_text(text, pos);
                std::match_results<std::string_view::const_iterator> smatch;
                if (std::regex_search(text.str().begin() + pos, text.str().end(), smatch, r))
                {
                    auto res = Match{pos, pos.add(text,smatch.length()), MatchType::regex_match};
                    return res;
                }
                else
                {
                    text.update_farthest_position(pos,MatchType::regex_match,s);
                    return std::nullopt;
                } });
        }

        inline auto sequence(std::vector<Pattern> patterns)
        {
            return rule([=](const Config &config, ParserState &text, TextPosition pos) -> std::optional<Match>
                        {
                Match match{pos, pos, MatchType::sequence};
                for (auto pattern : patterns)
                {
                    auto sub_match = pattern(config, text, pos);
                    if (sub_match)
                    {
                        pos = sub_match.value().end;
                        match.end = pos;
                        match.children.push_back(sub_match.value());
                    }
                    else
                    {
                        return std::nullopt;
                    }
                }
                return match; });
        }

        inline auto ordered_choice(std::vector<Pattern> patterns)
        {
            return rule([=](const Config &config, ParserState &text, TextPosition pos) -> std::optional<Match>
                        {
                for (auto pattern : patterns)
                {
                    auto match = pattern(config, text, pos);
                    if (match)
                    {
                        return Match{.start = match.value().start, .end = match.value().end, .type = MatchType::ordered_choice, .children = {match.value()}};
                    }
                }
                return std::nullopt; });
        }

        inline auto negative_lookahead(Pattern pattern)
        {
            return rule([=](const Config &config, ParserState &text, TextPosition pos) -> std::optional<Match>
                        {
                auto match = pattern(config, text, pos);
                if (!match)
                {
                    return Match{.start=pos, .end=pos, .type=MatchType::negative_lookahead};
                }
                else
                {
                    return std::nullopt;
                } });
        }

        inline auto positive_lookahead(Pattern pattern)
        {
            return rule([=](const Config &config, ParserState &text, TextPosition pos) -> std::optional<Match>
                        {
                auto match = pattern(config, text, pos);
                if (match)
                {
                    return Match{.start=pos, .end=pos, .type=MatchType::positive_lookahead, .children={match.value()}};
                }
                else
                {
                    return std::nullopt;
                } });
        }

        inline auto one_or_more(Pattern pattern)
        {
            return rule([=](const Config &config, ParserState &text, TextPosition pos) -> std::optional<Match>
                        {
                auto sub_match = pattern(config, text, pos);
                if (!sub_match)
                {
                    return std::nullopt;
                }
                else
                {
                    auto match = Match{.start = sub_match.value().start, .end = sub_match.value().end, .type = MatchType::one_or_more, .children = {sub_match.value()}};
                    pos = match.end;
                    while (auto next_match = pattern(config, text, pos))
                    {
                        match.children.push_back(next_match.value());
                        pos = next_match.value().end;
                        match.end = pos;
                    }
                    return match;
                } });
        }

        inline auto zero_or_more(Pattern pattern)
        {
            return rule([=](const Config &config, ParserState &text, TextPosition pos) -> std::optional<Match>
                        {
                auto match = Match{.start = pos, .end = pos, .type = MatchType::zero_or_more, .children = {}};
                while (auto next_match = pattern(config, text, pos))
                {
                    match.children.push_back(next_match.value());
                    pos = next_match.value().end;
                    match.end = pos;
                }
                return match; });
        }

        inline auto end_of_file()
        {
            return rule([=](const Config &config, ParserState &text, TextPosition pos) -> std::optional<Match>
                        {
                pos = config.skip_text(text, pos);
                if (pos==text.length()) {
                    return Match{pos,pos,MatchType::end_of_file};
                }
                else {
                    text.update_farthest_position(pos,MatchType::end_of_file,"");
                    return std::nullopt;
                } });
        }

        namespace skip_text_functions
        {
            inline auto skip_pattern(Pattern p)
            {
                return [=](ParserState &text, TextPosition pos) -> TextPosition
                {
                    static Config empty_config{.skip_text = nothing()};
                    std::optional<Match> match;
                    while ((pos < text.length()) && (match = p(empty_config, text, pos)))
                    {
                        pos = match.value().end;
                    }
                    return pos;
                };
            }
            inline auto skip_cpp_line_comments() {
                return textx::arpeggio::skip_text_functions::skip_pattern(
                    textx::arpeggio::regex_match(R"(//.*?(?:$|\n))")
                );
            }
            inline auto skip_cpp_multiline_comments() {
                return textx::arpeggio::skip_text_functions::skip_pattern(
                    textx::arpeggio::regex_match(R"(/\*(?:.|\n)*?\*/)")
                );
            }
            inline auto skip_cpp_style() {
                return textx::arpeggio::skip_text_functions::combine({
                    skipws(),
                    skip_cpp_line_comments(),
                    skip_cpp_multiline_comments()
                });
            }

        }

    }
}