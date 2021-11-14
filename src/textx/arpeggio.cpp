#include "textx/arpeggio.h"

namespace textx
{
    namespace arpeggio
    {
        std::unordered_map<MatchType, std::string> Match::type2str = {
            {MatchType::undefined, "undefined"},
            {MatchType::str_match, "str_match"},
            {MatchType::regex_match, "regex_match"},
            {MatchType::sequence, "sequence"},
            {MatchType::ordered_choice, "ordered_choice"},
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
            {MatchType::negative_lookahead, false},
            {MatchType::positive_lookahead, false},
            {MatchType::one_or_more, false},
            {MatchType::zero_or_more, false},
            {MatchType::optional, false},
            {MatchType::end_of_file, true}, // true? special case?
            {MatchType::custom, false},
        };

        size_t ParserState::cache_reset_indicator_source = 1; 

    }
}