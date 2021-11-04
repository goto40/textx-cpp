#include "textx/arpeggio.h"

namespace textx
{
    namespace arpeggio
    {
        std::unordered_map<MatchType, std::string> Match::type2str = {
            {MatchType::str_match, "str_match"},
            {MatchType::regex_match, "regex_match"},
            {MatchType::sequence, "sequence"},
            {MatchType::ordered_choice, "ordered_choice"},
            {MatchType::not_this, "not_this"},
            {MatchType::one_or_more, "one_or_more"},
            {MatchType::zero_or_more, "zero_or_more"},
        };

        std::unordered_map<MatchType, bool> Match::is_terminal = {
            {MatchType::str_match, true},
            {MatchType::regex_match, true},
            {MatchType::sequence, false},
            {MatchType::ordered_choice, false},
            {MatchType::not_this, false},
            {MatchType::one_or_more, false},
            {MatchType::zero_or_more, false},
        };

    }
}