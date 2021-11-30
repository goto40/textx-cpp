#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/metamodel.h"

TEST_CASE("model_simple1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: NUMBER|STRING;
        )";

        {
            textx::Metamodel mm{grammar1};
            CHECK(mm.parsetree_from_str("123"));
            CHECK(mm.parsetree_from_str("'123'"));
            CHECK_THROWS(mm.model_from_str("123"));
            CHECK_THROWS(mm.model_from_str("'123'"));
        }
        {
            // model only work from a shared metamodel...
            auto mm = std::make_shared<textx::Metamodel>(grammar1);
            CHECK(mm->model_from_str("123"));
            CHECK(mm->model_from_str("'123'"));
        }
    }
}

TEST_CASE("model_simple2", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: x=NUMBER|x=STRING;
        )";

        {
            // model only work from a shared metamodel...
            auto mm = std::make_shared<textx::Metamodel>(grammar1);
            auto m1 = mm->model_from_str("123");
            auto m2 = mm->model_from_str("'123'");
            CHECK(m1);
            CHECK(m2);
        }
    }
}
