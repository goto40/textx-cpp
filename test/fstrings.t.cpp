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