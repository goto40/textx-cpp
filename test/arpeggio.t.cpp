#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/arpeggio.h"

TEST_CASE("str_match", "[arpeggio]")
{
    using namespace textx::arpeggio;

    std::string text = "hello world";
    auto hello_pattern = str_match("hello");
    auto world_pattern = str_match("world");
    CHECK(hello_pattern(text, 0));
    CHECK(!world_pattern(text, 0));
    auto match = hello_pattern(text, 0).value();
    CHECK(world_pattern(text, match.end + 1));
}

TEST_CASE("named", "[arpeggio]")
{
    using namespace textx::arpeggio;

    std::string text = "hello world";
    auto hello_pattern = str_match("hello");
    auto named_hello_pattern = named("hello", str_match("hello"));
    auto match = hello_pattern(text, 0).value();
    auto named_match = named_hello_pattern(text, 0).value();
    {
        std::ostringstream o;
        o << match;
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<str_match>"));
    }
    {
        std::ostringstream o;
        o << named_match;
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<str_match:hello>"));
    }
}
