#include "textx/arpeggio.h"

namespace textx
{
    namespace arpeggio
    {
        std::unordered_map<MatchType, std::string> Match::type2str = {
            {MatchType::str_match, "str_match"},
        };
    }
}