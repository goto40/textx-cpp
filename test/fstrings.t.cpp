#include "catch.hpp"
#include "textx/fstrings.h"

TEST_CASE("fstrings_metamodel0", "[textx/fstrings]")
{
    auto model_text = R"(
        Hello World
        123
    )";
    auto m = textx::fstrings::get_fstrings_metamodel()->model_from_str(model_text);
    CHECK( (*m)["parts"].size()==0 );
    std::ostringstream s;
    for (auto &t: (*m)["text"]) {
        s << t["text"].str();
    }
    CHECK( s.str() == model_text );
    //std::cout << m->val() << "\n";
}

TEST_CASE("fstrings_metamodel1", "[textx/fstrings]")
{
    auto model_text = R"(
        Hello World {% model.x %}
        123
    )";
    auto m = textx::fstrings::get_fstrings_metamodel()->model_from_str(model_text);
    std::ostringstream s;
    for (auto &p: (*m)["parts"]) {
        for (auto &t: p["text"]) {
            s << t["text"].str();
        }
    }
    for (auto &t: (*m)["text"]) {
        s << t["text"].str();
    }
    CHECK( s.str().find("Hello World")>0 );
    CHECK( s.str().find("123")>0 );
    //std::cout << m->val() << "\n";
}