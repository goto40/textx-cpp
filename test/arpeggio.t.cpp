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
    auto match2_opt = world_pattern(text, match.end + 1);
    REQUIRE(match2_opt);

    CHECK(get_str(text, match)=="hello");
    CHECK(get_str(text, match2_opt.value())=="world");
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

TEST_CASE("captured", "[arpeggio]")
{
    using namespace textx::arpeggio;

    std::string text = "hello world";
    auto hello_pattern = capture(str_match("hello"));
    auto named_hello_pattern = capture(named("hello", str_match("hello")));
    auto match = hello_pattern(text, 0).value();
    auto named_match = named_hello_pattern(text, 0).value();
    {
        std::ostringstream o;
        o << match;
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<str_match captured=hello>"));
    }
    {
        std::ostringstream o;
        o << named_match;
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<str_match:hello captured=hello>"));
    }
}

TEST_CASE("regex_match", "[arpeggio]")
{
    using namespace textx::arpeggio;

    std::string text = "hello123 world";
    auto word_pattern = capture(regex_match(R"(\w+)"));
    {
        CHECK(!word_pattern(" ",1));

        auto match = word_pattern(text,0).value();
        std::ostringstream o;
        o << match;
        CHECK_THAT(o.str(), Catch::Matchers::Contains("<regex_match captured=hello123>"));
    }
}
