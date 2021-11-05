#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/lang.h"

TEST_CASE("simple1", "[textx/lang]")
{
    using namespace textx::arpeggio;

    auto grammar1 = R"(
        Model: A|B;
        A: 'A';
        B: 'B';
    )";

    //Grammar textx{textx::lang::textx_model()};
    //CHECK(textx.parse(grammar1));
}
