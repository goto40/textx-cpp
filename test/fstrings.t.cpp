#include "catch.hpp"
#include "textx/fstrings.h"

namespace {
    auto get_example_model() {
        auto mm = textx::metamodel_from_str(R"#(
            Model: shapes+=Shape[','];
            Shape: Point|ComplexShape;
            ComplexShape: Circle|Line;
            Point: type_name='Point' '(' x=NUMBER ',' y=NUMBER ')';
            Circle: type_name='Circle' '(' center=Point ',' r=NUMBER ')';
            Line: type_name='Line' '(' p1=Point ',' p2=Point ')';
        )#");
        auto m = mm->model_from_str(R"(
            Point(1,2),
            Circle(Point(333,4.5),9),
            Line(Point(0,0),Point(1,1))
        )");
        return m;
    }
}

TEST_CASE("fstrings_metamodel0", "[textx/fstrings]")
{
    auto model_text = R"(
        Hello World
        123
    )";
    auto m = textx::fstrings::get_fstrings_metamodel_workspace()->model_from_str(model_text);
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
    auto mm = textx::fstrings::get_fstrings_metamodel_workspace();
    mm->get_metamodel_by_shortcut("FSTRINGS")->clear_builtin_models();
    mm->get_metamodel_by_shortcut("FSTRINGS")->add_builtin_model(
        mm->model_from_str("EXTERNAL_LINKAGE","object model")
    );
    auto m = mm->model_from_str(model_text);
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

TEST_CASE("fstrings_metamodel2", "[textx/fstrings]")
{
    auto model = get_example_model();
    auto res = textx::fstrings::f(
        R"(
            Hello World {% model.x %}
            123
        )",
        { {"model", model} }
    );

    CHECK( res.size()>0 );
}