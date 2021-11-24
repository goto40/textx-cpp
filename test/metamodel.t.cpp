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
            Model: ('A' 'B' 'C')#;
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

TEST_CASE("metamodel_simple_expression5_regex", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: "[" /\w+/ "]";
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.model_from_str("[hello]"));
        CHECK_THROWS(mm.model_from_str("[hello world]"));
        CHECK_THROWS(mm.model_from_str("[]"));
    }
}

TEST_CASE("metamodel_simple_expression6_neg_lookahead", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: "[" !'key' /\w+/ "]";
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.model_from_str("[hello]"));
        CHECK_THROWS(mm.model_from_str("[key]"));
    }
}

TEST_CASE("metamodel_simple_expression6_pos_lookahead", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: "[" &('a'|'b'|'extra') /\w+/ "]";
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.model_from_str("[a]"));
        CHECK(mm.model_from_str("[best]"));
        CHECK(mm.model_from_str("[extra]"));
        CHECK(mm.model_from_str("[extra123]"));
        CHECK_THROWS(mm.model_from_str("[xyz]"));
    }
}

TEST_CASE("metamodel_simple_assignment1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'value' '=' value=/\w+/;
        )";

        textx::Metamodel mm{grammar1};
        CHECK_THROWS(mm.model_from_str("value="));
        CHECK(mm.model_from_str("value=Hello"));
        CHECK_THROWS(mm.model_from_str("value=Hello World"));
    }
}

TEST_CASE("metamodel_simple_assignment2", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'value' '=' value+=/\w+/;
        )";

        textx::Metamodel mm{grammar1};
        CHECK_THROWS(mm.model_from_str("value="));
        CHECK(mm.model_from_str("value=Hello"));
        CHECK(mm.model_from_str("value=Hello World"));
    }
}

TEST_CASE("metamodel_simple_assignment3", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'value' '=' value+=/\w+/[','];
        )";

        textx::Metamodel mm{grammar1};

        CHECK_THROWS(mm.model_from_str("value="));
        CHECK(mm.model_from_str("value=Hello"));
        CHECK(mm.model_from_str("value=Hello, World"));
        CHECK_THROWS(mm.model_from_str("value=Hello World"));
    }
    {
        auto grammar1 = R"(
            Model: 'value' '=' value*=/\w+/[','];
        )";

        textx::Metamodel mm{grammar1};

        CHECK(mm.model_from_str("value="));
        CHECK(mm.model_from_str("value=Hello"));
        CHECK(mm.model_from_str("value=Hello, World"));
        CHECK_THROWS(mm.model_from_str("value=Hello World"));
    }
}
