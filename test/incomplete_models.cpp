#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/metamodel.h"
#include "textx/workspace.h"

TEST_CASE("incomplete_model_with_point_negative_with_exception", "[textx/metamodel]")
{
    {
        auto mm = textx::metamodel_from_str(R"#(
            Model: points+=Point[','];
            Point: "(" x=NUMBER "," y=NUMBER ")";
        )#");
        CHECK_THROWS(mm->model_from_str("(1,2), (3,4.5), (9,"));
    }
}

TEST_CASE("incomplete_model_with_point", "[textx/metamodel]")
{
    {
        auto workspace = textx::Workspace::create();
        workspace->set_throw_on_problem(false);
        workspace->add_metamodel_from_str_for_extension("model", "mygrammar.tx", 
            R"#(
                Model: points+=Point[','];
                Point: "(" x=NUMBER "," y=NUMBER ")";
            )#");

        {
            auto m0 = workspace->model_from_str("mygrammar", "(1,2), (3,4.5), (9,1)");
            CHECK(m0->ok());
        }
        {
            auto m1 = workspace->model_from_str("mygrammar", "(1,2), (3,4.5), (9,");
            CHECK(!m1->ok());

            CHECK(std::get<std::shared_ptr<textx::object::Object>>((*m1)["points"][0].data)->type == "Point");
            CHECK((*m1)["points"].size() == 2);
            CHECK((*m1)["points"][0]["x"].i() == 1);
            CHECK((*m1)["points"][0]["y"].i() == 2);
            CHECK((*m1)["points"][1]["x"].i() == 3);
            CHECK((*m1)["points"][1]["y"].str() == "4.5");
        }
    }
}

TEST_CASE("incomplete_model_with_refs", "[textx/metamodel]")
{
    {
        auto workspace = textx::Workspace::create();
        workspace->set_throw_on_problem(false);
        workspace->add_metamodel_from_str_for_extension("model", "mygrammar.tx", 
            R"#(
                Model: refs+=Ref;
                Ref: name=ID ref=[Ref] ';';
            )#");

        {
            auto m0 = workspace->model_from_str("mygrammar", "a a;");
            CHECK(m0->ok());
        }
        {
            auto m0 = workspace->model_from_str("mygrammar", "a b; b a;");
            CHECK(m0->ok());
        }
        {
            auto m1 = workspace->model_from_str("mygrammar", "a a; b b; c d;");
            CHECK(!m1->ok());

            CHECK((*m1)["refs"].size() == 3);
            CHECK(!(*m1)["refs"][0]["ref"].is_null());
            CHECK(!(*m1)["refs"][1]["ref"].is_null());
            CHECK((*m1)["refs"][2]["ref"].is_null());
            CHECK(m1->get_errors().size()==1);
            CHECK_THAT(m1->get_errors()[0].error, Catch::Matchers::Contains("'d' not found"));
        }
    }
}
