#pragma once
#include "textx/grammar.h"

namespace textx
{
    /** adapted grammar from lang.py */
    namespace lang
    {
        struct TextxGrammar : textx::Grammar<> {            
           TextxGrammar();
        };
    }
}
