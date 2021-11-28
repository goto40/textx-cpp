#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/metamodel.h"

TEST_CASE("model_simple1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: variables+=Variable;
            Variable: name=ID "=" value=Value;
            Value: NUMBER|STRING;
        )";

        textx::Metamodel mm{grammar1};
        auto m = mm.model_from_str("x=1 y='Hello'");

    }
}
