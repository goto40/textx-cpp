#include "catch.hpp"
#include "itemlang.h"

TEST_CASE("model_simple1", "[itemlang]")
{
    auto model_text = R"(
        package simple

        struct Point {
            scalar x: built_in.float
            scalar y: built_in.float
        }

        struct Polygon {
            scalar n: built_in.uint32 (.maxValue=7, .defaultValue=3)
            array p: Point[n] (.fixedSizeInBytes=20)
            array info: built_in.char[256] (.defaultStringValue="your favorite polygon")
        }
    )";

    auto workspace = itemlang::get_workspace();
    auto m = workspace->model_from_str(model_text);
    //std::cout << m->val() << "\n";
    CHECK( m->val()["packages"].size()==1 );
}