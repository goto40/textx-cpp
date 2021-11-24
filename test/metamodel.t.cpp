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

        CHECK(mm["Model"]["value"].cardinality == textx::AttributeCardinality::scalar);
        //std::cout << mm << "\n";
    }
}

TEST_CASE("metamodel_simple_assignment1_repeated", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'value' '=' (value=/\w+/)*;
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.model_from_str("value="));
        CHECK(mm.model_from_str("value=Hello"));
        CHECK(mm.model_from_str("value=Hello World"));

        CHECK(mm["Model"]["value"].cardinality == textx::AttributeCardinality::list);
        //std::cout << mm << "\n";
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

        //std::cout << mm << "\n";
        CHECK(mm["Model"]["value"].cardinality == textx::AttributeCardinality::list);
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

        CHECK(mm["Model"]["value"].cardinality == textx::AttributeCardinality::list);
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

        CHECK(mm["Model"]["value"].cardinality == textx::AttributeCardinality::list);
    }
}

TEST_CASE("metamodel_simple_unordered_group_test1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: ('A' 'B' 'C')#;
        )";

        textx::Metamodel mm{grammar1};

        CHECK_THROWS(mm.model_from_str("AB"));
        CHECK(mm.model_from_str("ABC"));
        CHECK(mm.model_from_str("BCA"));
        CHECK(mm.model_from_str("C B A"));
        CHECK_THROWS(mm.model_from_str("A,B,C"));
    }
}

TEST_CASE("metamodel_simple_unordered_group_test2", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: ('A' 'B' 'C')#[','];
        )";

        textx::Metamodel mm{grammar1};
        
        CHECK_THROWS(mm.model_from_str("A,B"));
        CHECK_THROWS(mm.model_from_str("ABC"));
        CHECK(mm.model_from_str("A,B,C"));
        CHECK(mm.model_from_str("B,C,A"));
        CHECK(mm.model_from_str("C,B,A"));
        CHECK_THROWS(mm.model_from_str("A,B,"));
    }
}

TEST_CASE("metamodel_simple_unordered_group_test3", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: ('A' 'B' 'C')#[/\d*/];
        )";

        textx::Metamodel mm{grammar1};
        
        CHECK_THROWS(mm.model_from_str("A1B"));
        CHECK(mm.model_from_str("ABC"));
        CHECK(mm.model_from_str("A1B2C"));
        CHECK(mm.model_from_str("B3C123A"));
        CHECK(mm.model_from_str("CB1A"));
        CHECK_THROWS(mm.model_from_str("A1B2"));
    }
}
