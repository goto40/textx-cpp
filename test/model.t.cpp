#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/metamodel.h"

TEST_CASE("model_simple1", "[textx/model]")
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
            auto mm = textx::metamodel_from_str(grammar1);
            auto m1 = mm->model_from_str("123");
            auto m2 = mm->model_from_str("'123'");
            CHECK(m1->val()["x"].i() == 123);
            CHECK(m2->val()["x"].str() == "'123'");
        }
    }
}

TEST_CASE("model_simple3", "[textx/metamodel]")
{
    {
        auto grammar1 = R"#(
            Model: points+=Point[','];
            Point: "(" x=NUMBER "," y=NUMBER ")";
        )#";

        {
            auto mm = textx::metamodel_from_str(grammar1);
            auto m1 = mm->model_from_str("(1,2), (3,4.5)");
            CHECK(std::get<std::shared_ptr<textx::object::Object>>(m1->val()["points"][0].data)->type == "Point");
            CHECK(m1->val()["points"].size() == 2);
            CHECK(m1->val()["points"][0]["x"].i() == 1);
            CHECK(m1->val()["points"][0]["y"].i() == 2);
            CHECK(m1->val()["points"][1]["x"].i() == 3);
            CHECK(m1->val()["points"][1]["y"].str() == "4.5");
            //TODO: check if int was correctly converted: 
            // CHECK_THROWS(m1->val()["points"][1]["y"].i() == 4);
        }
    }
}

TEST_CASE("model_abstract_rule1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"#(
            Model: shapes+=Shape[','];
            Shape: Point|Circle|Line;
            Point: 'Point' '(' x=NUMBER ',' y=NUMBER ')';
            Circle: 'Circle' '(' center=Point ',' r=NUMBER ')';
            Line: 'Line' '(' p1=Point ',' p2=Point ')';
        )#";

        {
            auto mm = textx::metamodel_from_str(grammar1);
            auto m = mm->model_from_str("Point(1,2), Circle(Point(333,4.5),9), Line(Point(0,0),Point(1,1))");
            CHECK( (*mm)["Model"]["shapes"].cardinality == textx::AttributeCardinality::list );
            CHECK( (*mm)["Model"]["shapes"].type.value() == "Shape" );
            CHECK( m->val()["shapes"].size() == 3 );
            CHECK( m->val()["shapes"][0].obj()->type == "Point" );
            CHECK( m->val()["shapes"][1].obj()->type == "Circle" );
            CHECK( m->val()["shapes"][1]["center"]["x"].i() == 333 );
            CHECK( m->val()["shapes"][2].obj()->type == "Line" );
        }
    }
}

TEST_CASE("model_ref1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"#(
            Model: items+=Item 'ref' ref=[Item];
            Item: 'item' name=ID;
        )#";

        {
            auto mm = textx::metamodel_from_str(grammar1);
            auto m = mm->model_from_str("item A item B item C ref B");
            CHECK( m->val()["items"].size() == 3 );
            CHECK( m->val()["items"][0]["name"].str() == "A" );
            CHECK( m->val()["items"][1]["name"].str() == "B" );
            CHECK( m->val()["items"][2]["name"].str() == "C" );
            CHECK( m->val()["ref"].is_ref() );
            CHECK( m->val()["ref"].ref().name == "B" ); // the reference string (the identifier to find the obj)
            CHECK( m->val()["ref"]["name"].str() == "B" ); // the name of the (found) referenced element
            CHECK( m->val()["ref"].obj() == m->val()["items"][1].obj() );
        }
    }
}
