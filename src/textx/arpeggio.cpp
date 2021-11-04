#include "textx/arpeggio.h"

namespace textx
{
    namespace arpeggio
    {
        std::unordered_map<MatchType, std::string> Match::type2str = {
            {MatchType::str_match, "str_match"},
            {MatchType::regex_match, "regex_match"},
            {MatchType::sequence, "sequence"},
        };

        std::unordered_map<MatchType, bool> Match::is_terminal = {
            {MatchType::str_match, true},
            {MatchType::regex_match, true},
            {MatchType::sequence, false},
        };

    }
}