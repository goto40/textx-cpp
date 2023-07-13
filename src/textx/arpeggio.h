#pragma once

#define ARPEGGIO_USE_BOOST_FOR_REGEX

#include <cstdlib>
#include <vector>
#include <string>
#include <optional>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <stdexcept>
#include <tuple>
#include <compare>
#include <functional>
#include <variant>
#include "assert.h"

#define DBG_TEXTX_ARPEGGIO(x)
#define DBG_TEXTX_ARPEGGIO_FOUND(x) DBG_TEXTX_ARPEGGIO(x)
namespace textx
{
    namespace arpeggio
    {
        struct Pattern;
        enum class MatchType
        {
            undefined,
            nomatch,
            end_of_file,
            str_match,
            regex_match,
            sequence,
            ordered_choice,
            unordered_group,
            negative_lookahead,
            positive_lookahead,
            one_or_more,
            zero_or_more,
            optional,
            custom,
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

            friend inline std::ostream &operator<<(std::ostream &o, const TextPosition &pos)
            {
                o << pos.line << ":" << pos.col;
                return o;
            }
        };

        struct Exception : std::exception {
            TextPosition pos;
            std::string error;
            std::string filename;
            Exception(std::string filename, TextPosition pos, std::string error) : pos{pos}, error{error}, filename{filename} {}
            const char* what() const noexcept override {
                return error.c_str();
            }
        };

        void print_error_position(std::ostream& s, std::string_view text, TextPosition pos); 

        template<class ...T>
        [[noreturn]] void raise(std::string_view text, std::string filename, TextPosition pos, T... params) {
            std::ostringstream o;
            if (text.size()>0) { 
                o << filename << ":" << pos << ": ";
                (o << ... << params) << "\n";
                textx::arpeggio::print_error_position(o, text, pos);
                o << "\n";
            }
            else {
                (o << ... << params) << "\n";
            }
            throw Exception{filename, pos, o.str()};
        }
        template<class ...T>
        [[noreturn]] void raise(TextPosition pos, T... params) {
            raise("", "", pos, params...);
        }

        class Match
        {
            TextPosition m_start, m_end;
            MatchType m_type;
            bool m_has_error;

        public:
            std::vector<Match> children = {};
            std::optional<std::string> name = std::nullopt;
            std::optional<std::string> captured = std::nullopt;
            bool has_error() { return m_has_error; }
            void set_error() { m_has_error = true; }

            Match(TextPosition s, TextPosition e, MatchType t, std::vector<Match> c = {}) : m_start{s}, m_end{e}, m_type{t}, children{c}
            {
                if (m_type == MatchType::undefined)
                {
                    raise(s, "unexpected: undefined match type...");
                }
            }

            const Match* search(std::string name) const;

            bool name_starts_with(const std::string_view p) const {
                return name.has_value() && name.value().starts_with(p);
            }

            bool name_is(const std::string_view p) const {
                return name.has_value() && name.value() == p;
            }
            auto start() const { return m_start; }
            auto end() const { return m_end; }
            auto type() const { return m_type; }
            void update_end(TextPosition e) { m_end = e; }
            template<class F>
            void traverse(F f) {
                f(*this);
                for (auto& c: children) c.traverse(f);
            }
            template<class F>
            void traverse(F f) const {
                f(*this);
                for (const auto& c: children) c.traverse(f);
            }

            static std::unordered_map<MatchType, std::string> type2str;
            static std::unordered_map<MatchType, bool> is_terminal;
            void print(std::ostream &o, size_t indent=0) const;
            friend inline std::ostream &operator<<(std::ostream &o, const Match &match)
            {
                match.print(o);
                return o;
            }
        };

        inline Match no_match(TextPosition pos) {
            Match m{pos, pos, MatchType::nomatch,{}};
            m.set_error();
            return m;
        }

        struct TextxErrorEntry {
            TextPosition pos;
            size_t length;
            std::string error;
        };

        class ParserResult {
            Match match;
            std::vector<TextxErrorEntry> m_errors;

        public:
            ParserResult(): match{no_match({})} {}
            ParserResult(const Match &m): match{m} {}
            ParserResult(Match &&m): match{m} {}
            static ParserResult error(std::string text, Match m) {
                ParserResult r{m};
                r.add_error({m.start(), m.end()-m.start(), text});
                return r;
            }
            static ParserResult error(std::string text, TextPosition pos) {
                return error(text, no_match(pos));
            }

            bool ok() const { return m_errors.size()==0; }
            operator bool() const { return ok(); }
            Match& value() { return match; }
            const Match& value() const { return match; }
            auto& operator*() { return value(); }
            const auto& operator*() const { return value(); }
            auto* operator->() { return &value(); }
            const auto* operator->() const { return &value(); }

            auto& errors() { return m_errors; }
            const auto& errors() const { return m_errors; }
            void add_error(TextxErrorEntry e) { m_errors.push_back(e); }
            void add_errors(const std::vector<TextxErrorEntry> e) { for(auto &x: e) m_errors.push_back(x); }

            inline ParserResult& update_match(Match m) {
                match = m;
                return *this; // forward error
            }

        };

        inline std::ostream& operator<<(std::ostream& o, MatchType t) {
            o<< Match::type2str.at(t);
            return o;
        }

        struct AnnotatedTextPosition
        {
            TextPosition text_position = {};
            std::vector<std::tuple<MatchType, std::string>> info;
            auto operator<=>(const AnnotatedTextPosition &b) const noexcept { return text_position <=> b.text_position; }
            operator TextPosition() { return text_position; }

            friend inline std::ostream &operator<<(std::ostream &o, const AnnotatedTextPosition &pos)
            {
                for (auto &i : pos.info)
                {
                    o << " -" << Match::type2str.at(std::get<MatchType>(i)) << "," << std::get<std::string>(i) << "\n";
                }
                return o;
            }
        };

        struct CompletionInfo {
            TextPosition pos;
            std::function<std::vector<std::string>()> info;
        };

        class ParserState
        {
            std::string_view source;
            static size_t cache_reset_indicator_source;
            size_t cache_reset_indicator = {cache_reset_indicator_source++};
            std::string m_filename;
            std::unordered_map<size_t, std::vector<CompletionInfo>> completionInfo;            

        public:
            bool eolterm = false;
            bool skipws = true;
            size_t cache_hits = {0};
            size_t cache_misses = {0};
            AnnotatedTextPosition farthest_position = {};

            size_t get_cache_reset_indicator() { return cache_reset_indicator; }
            ParserState(std::string_view s, std::string filename) : source(s), m_filename{filename} {}
            operator std::string_view() { return source; }
            size_t length() { return source.length(); }
            char operator[](size_t p) { return source[p]; }
            void update_farthest_position(TextPosition pos, MatchType type, std::string_view info);
            std::string_view str() { return source; }
            const std::string& filename() { return m_filename; }
            void add_completion_info(TextPosition pos, std::function<std::vector<std::string>()> f);
            bool has_completion_info(size_t pos) { return completionInfo.contains(pos); }
            const auto& get_completion_info() const { return completionInfo; }
            auto& get_completion_info(size_t pos) const {
                auto it = completionInfo.find(pos);
                TEXTX_ASSERT(it!=completionInfo.end());
                return *it;
            }
            std::string get_last_error_string(std::optional<std::string_view> text = std::nullopt)
            {
                std::ostringstream s;
                // if (text) {
                //     textx::arpeggio::print_error_position(s, text.value(), pos.text_position);
                // }
                s << farthest_position.text_position.line << ":" << farthest_position.text_position.col << ":"
                << "expected\n" << farthest_position;
                return s.str();
            }
        };

        using SkipTextFun = std::function<TextPosition(ParserState &text, TextPosition pos)>;

        namespace skip_text_functions
        {
            inline SkipTextFun nothing()
            {
                return [](ParserState &, TextPosition pos) -> TextPosition
                {
                    return pos;
                };
            }
            inline SkipTextFun skipws()
            {
                return [=](ParserState &text, TextPosition pos) -> TextPosition
                {
                    if (text.skipws) {
                        auto isspace = [&](char c) -> bool {
                            if (!text.eolterm) return std::isspace(c);
                            else return std::isspace(text[pos]) && c!='\n' && c!='\r';
                        };
                        while (pos < text.length() && isspace(text[pos]))
                        {
                            pos.inc(text);
                        }
                    }
                    return pos;
                };
            }
            inline SkipTextFun combine(std::vector<SkipTextFun> ps)
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
            SkipTextFun skip_pattern(textx::arpeggio::Pattern p);
            SkipTextFun skip_cpp_line_comments();
            SkipTextFun skip_cpp_multiline_comments();
            SkipTextFun skip_cpp_style();
        }

        struct Config
        {
            SkipTextFun skip_text = skip_text_functions::skipws();
        };

        using PatternFunc = std::function<ParserResult(const Config &config, ParserState &text, TextPosition pos)>;
        class Pattern {
            PatternFunc patternfunc;
            MatchType type_value;
        public:
            Pattern() : patternfunc{}, type_value{MatchType::undefined} {}
            template<class Func>
            Pattern(Func f, MatchType t=MatchType::custom) : patternfunc{f}, type_value{t} {}
            ParserResult operator()(const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) const {
                return patternfunc(config, text, pos);
            }
            MatchType type() { return type_value; }
        };

        inline std::string_view get_str(std::string_view text, Match match)
        {
            return text.substr(match.start(), match.end() - match.start());
        }

        inline bool is_terminal(const Match &m)
        {
            return Match::is_terminal.at(m.type());
        }

        // decorator
        /**
         * This function is a rule wrapper requried for each rule.
         * It 
         *  - manages memoization 
         *  - does basic checks
         *  - skip whitespaces if enabled (skipws/noskipws)
         *
         * If you omit the rule-call in your high-level grammar you loose some efficiency.
         */
        inline Pattern rule(Pattern pattern)
        {
            return {[=, cache = std::unordered_map<size_t, ParserResult>{}, chached_state = static_cast<size_t>(0)](const Config &config, ParserState &text, TextPosition pos) mutable -> ParserResult
            {
                // basic checks:
                if (pos > text.length())
                {
                    throw std::runtime_error("unexpected: pos>text.length()");
                }
                if (pattern.type()!=MatchType::optional && pattern.type()!=MatchType::zero_or_more) {
                    pos = config.skip_text(text, pos);
                }
                
                // memoization:
                if (chached_state != text.get_cache_reset_indicator())
                {
                    chached_state = text.get_cache_reset_indicator();
                    cache = {};
                }
                if (cache.count(pos.pos) > 0)
                {
                    text.cache_hits++;
                    return cache[pos.pos];
                }
                else
                {
                    text.cache_misses++;
                    cache[pos.pos] = ParserResult::error("recursion breaker (internal)", no_match(pos)); // recursion breaker

                    auto match = pattern(config, text, pos);

                    if (match)
                    {
                        if (match.value().type() == MatchType::undefined)
                        {
                            std::ostringstream s;
                            s << "unexpected, found undefined result = " << match.value();
                            raise(match->start(), s.str());
                        }
                    }
                    cache[pos.pos] = match;
                    return match;
                }
            }, pattern.type()}; // forward type
        }

        // decorator
        inline Pattern named(std::string name, Pattern pattern)
        {
            return {[=](const Config &config, ParserState &text, TextPosition pos) -> ParserResult
            {
                //std::cout << "NAMED RULE " << name << "\n";
                auto parseResult = pattern(config, text, pos);
                if (parseResult.ok())
                {
                    parseResult.value().name = name;
                    DBG_TEXTX_ARPEGGIO_FOUND(std::cout << "TEXTX DBG found name:" << name << " @" << parseResult.value().start() << "\n";)
                }
                else
                {
                    text.update_farthest_position(pos,MatchType::str_match,std::string("rule-name="+name)+(text.eolterm?"+eolterm":""));
                }
                return parseResult; }, pattern.type()};
        }

        // decorator
        inline Pattern capture(Pattern pattern)
        {
            return {[=](const Config &config, ParserState &text, TextPosition pos) -> ParserResult
            {
                auto parseResult = pattern(config, text, pos);
                if (parseResult.ok())
                {
                    parseResult.value().captured = get_str(text, parseResult.value());
                    DBG_TEXTX_ARPEGGIO_FOUND(std::cout << "TEXTX DBG found captured:" << parseResult.value().captured.value() << " @" << parseResult.value().start() << "\n";)
                }
                return parseResult; }, pattern.type()};
        }

        inline Pattern modify_parser_state(std::function<void(ParserState &)> modifier, Pattern pattern)
        {
            return {[=](const Config &config, ParserState &text, TextPosition pos) -> ParserResult
                        {
                auto copy = text;
                modifier(text);
                auto parseResult = pattern(config, text, pos);
                // preserve error infos:
                copy.farthest_position = text.farthest_position;
                text = copy;
                return parseResult;
                        }, pattern.type()};
        }

        inline auto eolterm(Pattern pattern)
        {
            return modify_parser_state([](ParserState &s){
                s.eolterm = true;
            }, pattern);
        }

        inline auto skipws(Pattern pattern)
        {
            return modify_parser_state([](ParserState &s){
                s.skipws = true;
            }, pattern);
        }

        inline auto noskipws(Pattern pattern)
        {
            return modify_parser_state([](ParserState &s){
                s.skipws = false;
            }, pattern);
        }

        namespace details {
            std::vector<bool> get_is_optional(std::vector<Pattern> patterns);
        }

        Pattern optional(Pattern pattern);
        Pattern str_match(std::string s);
        Pattern regex_match(std::string s);
        Pattern sequence(std::vector<Pattern> patterns);
        Pattern ordered_choice(std::vector<Pattern> patterns);
        Pattern unordered_group(std::vector<Pattern> patterns, std::optional<Pattern> separator=std::nullopt);
        Pattern negative_lookahead(Pattern pattern);
        Pattern positive_lookahead(Pattern pattern);
        Pattern one_or_more(Pattern pattern);
        Pattern zero_or_more(Pattern pattern);
        Pattern end_of_file();

    }
}