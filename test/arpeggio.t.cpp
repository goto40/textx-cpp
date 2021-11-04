#include "catch.hpp"
#include <iostream>
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
