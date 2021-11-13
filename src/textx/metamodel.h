#pragma once
#include "textx/lang.h"
#include "textx/grammar.h"
#include "textx/rule.h"
#include <string>

namespace textx {

    class Metamodel {
        textx::lang::TextxGrammar textx_grammar={};
        textx::Grammar<textx::Rule> grammar={};
        std::optional<textx::arpeggio::Match> grammar_root=std::nullopt;
        public:
        Metamodel(std::string_view grammar);
    };

}