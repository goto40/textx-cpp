#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/lang.h"

TEST_CASE("simple1", "[textx/lang]")
{
    using namespace textx::arpeggio;
    using namespace textx;
    {
        auto grammar1 = R"(
            Model: 'hello';
        )";

        textx::lang::TextxGrammar textx_grammar;
        CHECK(textx_grammar.parse(grammar1));
    }
    {
        auto grammar1 = R"(
            Model: A|B;
            A: 'A';
            B: 'B';
        )";
    }
}
