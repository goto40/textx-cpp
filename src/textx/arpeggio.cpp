// C++ regex seem to have problems with some regular expressions, e.g. in "FLOAT: ..."

#include "textx/arpeggio.h"
#include "textx/assert.h"
#ifdef ARPEGGIO_USE_BOOST_FOR_REGEX
#include <boost/regex.hpp>
#else
#include <regex>
#endif

namespace textx
{
    namespace arpeggio
    {
        std::unordered_map<MatchType, std::string> Match::type2str = {
            {MatchType::undefined, "undefined"},
            {MatchType::nomatch, "nomatch"},
            {MatchType::str_match, "str_match"},
            {MatchType::regex_match, "regex_match"},
            {MatchType::sequence, "sequence"},
            {MatchType::ordered_choice, "ordered_choice"},
            {MatchType::unordered_group, "unordered_group"},
            {MatchType::negative_lookahead, "negative_lookahead"},
            {MatchType::positive_lookahead, "positive_lookahead"},
            {MatchType::one_or_more, "one_or_more"},
            {MatchType::zero_or_more, "zero_or_more"},
            {MatchType::optional, "optional"},
            {MatchType::end_of_file, "end_of_file"},
            {MatchType::custom, "custom"},
        };

        std::unordered_map<MatchType, bool> Match::is_terminal = {
            {MatchType::undefined, false},
            {MatchType::str_match, true},
            {MatchType::regex_match, true},
            {MatchType::sequence, false},
            {MatchType::ordered_choice, false},
            {MatchType::unordered_group, false},
            {MatchType::negative_lookahead, false},
            {MatchType::positive_lookahead, false},
            {MatchType::one_or_more, false},
            {MatchType::zero_or_more, false},
            {MatchType::optional, false},
            {MatchType::end_of_file, true}, // true? special case?
            {MatchType::custom, false},
        };

        size_t ParserState::cache_reset_indicator_source = 1; 


        void ParserState::update_farthest_position(TextPosition pos, MatchType type, std::string_view info)
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

        void ParserState::add_completion_info(TextPosition pos, std::function<std::vector<std::string>()> f) {
            completionInfo[pos.pos].push_back({pos, f});
        }

        namespace skip_text_functions
        {
            textx::arpeggio::SkipTextFun skip_pattern(Pattern p)
            {
                return [=](ParserState &text, TextPosition pos) -> TextPosition
                {
                    static Config empty_config{.skip_text = nothing()};
                    ParserResult match;
                    while ((pos < text.length()) && (match = p(empty_config, text, pos)))
                    {
                        pos = match.value().end();
                    }
                    return pos;
                };
            }
            textx::arpeggio::SkipTextFun skip_cpp_line_comments()
            {
                return textx::arpeggio::skip_text_functions::skip_pattern(
                    textx::arpeggio::regex_match(R"(//.*?(?:$|\n))"));
            }
            textx::arpeggio::SkipTextFun skip_cpp_multiline_comments()
            {
                return textx::arpeggio::skip_text_functions::skip_pattern(
                    textx::arpeggio::regex_match(R"(/\*(?:.|\n)*?\*/)"));
            }
            textx::arpeggio::SkipTextFun skip_cpp_style()
            {
                return textx::arpeggio::skip_text_functions::combine({skipws(),
                                                                        skip_cpp_line_comments(),
                                                                        skip_cpp_multiline_comments()});
            }

        }

        const Match* Match::search(std::string name) const {
            if (name_is(name)) {
                return this;
            }
            else {
                for (auto &c: children) {
                    auto ret = c.search(name);
                    if (ret!=nullptr) { return ret; }
                }
            }
            return nullptr;
        }

        void Match::print(std::ostream &o, size_t indent) const
        {
            const Match &match = *this; 
            auto istr = std::string(indent, ' ');
            o << istr << "<" << type2str.at(match.type());
            if (match.name.has_value())
            {
                o << ":" << match.name.value();
            }
            if (match.captured.has_value())
            {
                o << " captured=" << match.captured.value();
            }
            o << ">(\n";
            for (auto &child : match.children)
            {
                child.print(o, indent+2);
            }
            o << istr << ")\n";
        }

        void print_error_position(std::ostream& s, std::string_view text, TextPosition pos) {
            const size_t p = pos.pos;
            const size_t n = std::min<size_t>(p, 30);
            const size_t m = std::min<size_t>(text.length()-p, 30);
            s << "..." << text.substr(pos.pos-n,n);
            s << "*\n" << text.substr(pos.pos,m);
            s << "...\n";
        }

        Pattern optional(Pattern pattern)
        {
            return rule({[=](const Config &config, ParserState &text, TextPosition pos) -> ParserResult
                        {
                    auto match = pattern(config, text, pos);
                    if (match.has_value())
                    {
                        return Match{match.value().start(), match.value().end(), MatchType::optional, {match.value()}};
                    }
                    else {
                        return Match{pos, pos, MatchType::optional, {}};
                    }
                }, MatchType::optional});
        }

        Pattern str_match(std::string s)
        {
            return rule({[=](const Config &config, ParserState &text, TextPosition pos) -> ParserResult
                        {
                if (text.str().substr(pos).starts_with(s))
                {
                    return Match{pos, pos.add(text, s.length()), MatchType::str_match};
                }
                else
                {
                    text.update_farthest_position(pos,MatchType::str_match,s);
                    text.add_completion_info(pos, [s](){return std::vector{s};});
                    return txError("str_match failure", text, pos, noMatch(pos));
                } }, MatchType::str_match});
        }

        Pattern regex_match(std::string s)
        {
#ifdef ARPEGGIO_USE_BOOST_FOR_REGEX
            using boost::match_results;
            using boost::regex;
            using boost::regex_search;
            //using boost::regex_constants::match_not_dot_newline;
            auto myregex = regex{s};
#else
            //TODO not working with certain regex situations...
            using std::match_results;
            using std::regex;
            using std::regex_search;
            auto myregex = regex{s};
#endif
            return rule({[=, r = myregex](const Config &config, ParserState &text, TextPosition pos) -> ParserResult
                        {
                match_results<std::string_view::const_iterator> smatch;
                if (regex_search(text.str().begin() + pos, text.str().end(), smatch, r))
                {
                    if (smatch.position() == 0) {
                        auto res = Match{pos, pos.add(text,smatch.length()), MatchType::regex_match};
                        return res;
                    }
                }
                // else - no match as index 0 found, no return so far...

                //text.update_farthest_position(pos,MatchType::regex_match,s);
                return txError("regex_match failure", text, pos, noMatch(pos)); },MatchType::regex_match});
        }

        Pattern sequence(std::vector<Pattern> patterns)
        {
            return rule({[=](const Config &config, ParserState &text, TextPosition pos) -> ParserResult
                        {
                Match match{pos, pos, MatchType::sequence};
                for (auto pattern : patterns)
                {
                    auto sub_match = pattern(config, text, pos);
                    if (sub_match)
                    {
                        pos = sub_match.value().end();
                        match.update_end(pos);
                        match.children.push_back(sub_match.value());
                    }
                    else
                    {
                        TEXTX_ASSERT(sub_match.err().match.has_value());
                        pos = sub_match.err().match.value().end();
                        match.update_end(pos);
                        match.children.push_back(sub_match.err().match.value());
                        return txForwardErrorAndUpdateMatch(sub_match, match);
                    }
                }
                return match; },MatchType::sequence});
        }

        Pattern ordered_choice(std::vector<Pattern> patterns)
        {
            return rule({[=](const Config &config, ParserState &text, TextPosition pos) -> ParserResult
                        {
                ParserResult error;
                for (auto pattern : patterns)
                {
                    auto match = pattern(config, text, pos);
                    if (match)
                    {
                        return Match{match.value().start(),match.value().end(),MatchType::ordered_choice, {match.value()}};
                    }
                    else {
                        txUpdateError(error, match);
                    }
                }
                TEXTX_ASSERT(error.err().errors.size()>0, "unexpected");
                return error; },MatchType::ordered_choice});
        }

        namespace details {
            std::vector<bool> get_is_optional(std::vector<Pattern> patterns) {
                std::vector<bool> is_optional={};
                std::transform(
                    patterns.begin(),
                    patterns.end(),
                    std::back_inserter(is_optional),
                    [](auto p)->bool{return p.type()==MatchType::optional;}
                );
                return is_optional;
            }
        }

        Pattern unordered_group(std::vector<Pattern> patterns, std::optional<Pattern> separator)
        {
            auto is_optional = details::get_is_optional(patterns);
            size_t optional_elements_n = std::count_if(is_optional.begin(), is_optional.end(), [](bool x){return x;});

            return rule({[=](const Config &config, ParserState &text, TextPosition pos) -> ParserResult
                        {
                ParserResult error;
                Match result{pos,pos,MatchType::unordered_group, {}};
                std::vector<bool> used(patterns.size());
                std::fill(used.begin(), used.end(), false);
                size_t n_req = 0;
                size_t n_opt = 0;
                size_t n = 0;

                for (size_t t=0;t<patterns.size();t++) {
                    for (size_t i=0;i<patterns.size();i++) {
                        if (!used[i]) {
                            bool ok = true;
                            TextPosition npos = pos;
                            if (n>0 && separator.has_value()) {
                                auto sep_match = separator.value()(config, text, pos);
                                if (sep_match) {
                                    npos = sep_match.value().end(); 
                                }
                                else {
                                    txUpdateError(error, sep_match);
                                    ok = false;
                                }
                            }
                            if (ok) {
                                auto match = patterns[i](config, text, npos);
                                if (match && is_optional[i] && match->start()==match->end()) {
                                    match = txError("empty optional match", text, npos, match.value()); // empty optional match
                                }
                                if (match)
                                {
                                    result.children.emplace_back(match.value());
                                    pos = match.value().end();
                                    used[i] = true;
                                    n++;
                                    if (is_optional[i]) n_opt++;
                                    else n_req++;
                                }
                                else {
                                    txUpdateError(error, match);                                    
                                }
                            }
                        }
                    }
                }
                result.update_end(pos);
                
                /* TODO remove comments below...

                "... If all elements are optional then even if there is no match of any of the 
                optional parts the whole match still succeeds. At least that's how it should
                work and a quick test confirms it. The only issue I noticed is that if the
                whole unordered match is contained in a single common rule than a reference 
                to that rule will return None in case all optionals are not matched. Is this 
                "the correct" behavior I don't know. It can be either this or to return an 
                object where all attributes are None. We can investigate this as a separate 
                issue if needed.
                */
                /*if (n==0) return std::nullopt; // special case, nothing was found
                else*/ 
                
                if (n_req == patterns.size()-optional_elements_n) return result;
                else return txForwardErrorAndUpdateMatch(error, result); }, MatchType::unordered_group});
        }

        Pattern negative_lookahead(Pattern pattern)
        {
            return rule({[=](const Config &config, ParserState &text, TextPosition pos) -> ParserResult
                        {
                auto match = pattern(config, text, pos);
                if (!match)
                {
                    return Match{pos, pos,MatchType::negative_lookahead};
                }
                else
                {
                    return txError("negative_lookahead failed", text, pos, noMatch(pos)); // forward match??
                } },MatchType::negative_lookahead});
        }

        Pattern positive_lookahead(Pattern pattern)
        {
            return rule({[=](const Config &config, ParserState &text, TextPosition pos) -> ParserResult
                        {
                auto match = pattern(config, text, pos);
                if (match)
                {
                    return Match{pos, pos, MatchType::positive_lookahead, {match.value()}};
                }
                else
                {
                    return txError("positive_lookahead failed", text, pos, noMatch(pos)); // forward match??
                } },MatchType::positive_lookahead});
        }

        Pattern one_or_more(Pattern pattern)
        {
            return rule({[=](const Config &config, ParserState &text, TextPosition pos) -> ParserResult
                        {
                auto sub_match = pattern(config, text, pos);
                if (!sub_match)
                {
                    return txForwardErrorAndUpdateMatch(sub_match, noMatch(pos));
                }
                else
                {
                    auto match = Match{sub_match.value().start(), sub_match.value().end(), MatchType::one_or_more, {sub_match.value()}};
                    pos = match.end();
                    while (auto next_match = pattern(config, text, pos))
                    {
                        match.children.push_back(next_match.value());
                        pos = next_match.value().end();
                        match.update_end(pos);
                    }
                    return match;
                } },MatchType::one_or_more});
        }
 
        Pattern zero_or_more(Pattern pattern)
        {
            return rule({[=](const Config &config, ParserState &text, TextPosition pos) -> ParserResult
                        {
                auto match = Match{pos, pos,MatchType::zero_or_more, {}};
                while (auto next_match = pattern(config, text, pos))
                {
                    match.children.push_back(next_match.value());
                    pos = next_match.value().end();
                    match.update_end(pos);
                }
                return match; },MatchType::zero_or_more});
        }

        Pattern end_of_file()
        {
            return rule({[=](const Config &config, ParserState &text, TextPosition pos) -> ParserResult
                        {
                if (pos==text.length()) {
                    return Match{pos,pos,MatchType::end_of_file};
                }
                else {
                    text.update_farthest_position(pos,MatchType::end_of_file,"");
                    return txError("end_of_file failure", text, pos, noMatch(pos));
                } },MatchType::end_of_file});
        }

    }
}