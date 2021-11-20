#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/metamodel.h"

TEST_CASE("metamodel_simple1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'simple model'; // the model is just a fixed string!
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.model_from_str("simple model"));
        CHECK_THROWS(mm.model_from_str("no model"));
    }
}
