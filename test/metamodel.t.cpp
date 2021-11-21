#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/metamodel.h"

TEST_CASE("metamodel_simple_expression1", "[textx/metamodel]")
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

TEST_CASE("metamodel_simple_expression2", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'A' 'B'* 'C'+;
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.model_from_str("ACCC"));
        CHECK(mm.model_from_str("ABBBC"));
        CHECK_THROWS(mm.model_from_str("A"));
    }
}

TEST_CASE("metamodel_simple_expression3", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: ('A'|'B')* 'C'+;
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.model_from_str("ACCC"));
        CHECK(mm.model_from_str("AABBBC"));
        CHECK(mm.model_from_str("BAC"));
        CHECK(mm.model_from_str("C"));
        CHECK_THROWS(mm.model_from_str("ABBACA"));
        CHECK_THROWS(mm.model_from_str(""));
    }
}

TEST_CASE("metamodel_simple_expression4", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: ('A'|'B'|'C')#;
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.model_from_str("ABC"));
        CHECK(mm.model_from_str("CAB"));
        CHECK(mm.model_from_str("CBA"));
        CHECK(mm.model_from_str("BAC"));
        CHECK_THROWS(mm.model_from_str("AAB"));
        CHECK_THROWS(mm.model_from_str("C"));
    }
}
