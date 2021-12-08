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
            CHECK(mm->model_from_str("123"));
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

            CHECK(mm->operator[]("Shape").tx_inh_by().size()==3);

            CHECK(mm->is_base_of("Shape","Point"));
            CHECK(mm->is_base_of("Point","Point"));
            CHECK(!mm->is_base_of("Point","Shape"));
            CHECK_THROWS(mm->is_base_of("Unknown","Point"));
            CHECK_THROWS(mm->is_base_of("Point","Unknown"));

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

TEST_CASE("model_ordered_choice_regression1", "[textx/model]")
{
    {
        auto grammar = R"(
            Model: ('A' 'B')#;
        )";
        {
            auto mm = textx::metamodel_from_str(grammar);
            CHECK(mm->model_from_str("A B"));
            CHECK(mm->model_from_str("B A"));
        }
    }
    {
        auto grammar = R"(
            Model: (a='A' b='B')#;
        )";
        {
            auto mm = textx::metamodel_from_str(grammar);
            CHECK(mm->model_from_str("A B"));
            CHECK(mm->model_from_str("B A"));
        }
    }
    {
        auto grammar = R"(
            Model: (a?='A' (b=BObj)?)#;
            BObj: x='B';
        )";
        {
            auto mm = textx::metamodel_from_str(grammar);
            {
                auto m = mm->model_from_str("A B");
                CHECK(m);
                CHECK(m->val()["a"].is_boolean());
                CHECK(m->val()["a"].boolean());
                CHECK(m->val()["b"].is_boolean());
                CHECK(m->val()["b"].boolean());
            }
            {
                auto m = mm->model_from_str("B A");
                CHECK(m);
                CHECK(m->val()["a"].is_boolean());
                CHECK(m->val()["a"].boolean());
                CHECK(m->val()["b"].is_boolean());
                CHECK(m->val()["b"].boolean());
            }
            CHECK(mm->model_from_str("B"));
            CHECK(mm->model_from_str("A"));
            auto m = mm->model_from_str("");
            CHECK(m);
            CHECK(m->val()["a"].is_boolean());
            CHECK(!m->val()["a"].boolean());
            CHECK(m->val()["b"].is_boolean());
            CHECK(!m->val()["b"].boolean());
        }
    }
}

TEST_CASE("model_ordered_choice2", "[textx/model]")
{
    std::ostringstream text1, text2;
    {
        auto mm = textx::metamodel_from_str("Model: ('A'|('B' 'C'))#;");
        auto m = mm->model_from_str("B C A");
        text1 << m << "\n";
        CHECK(m);
        CHECK(mm->model_from_str("A B C"));
        CHECK_THROWS(mm->model_from_str("B A C"));
    }
    {
        auto mm = textx::metamodel_from_str("Model: ('A' ('B' 'C'))#;");
        auto m = mm->model_from_str("B C A");
        text2 << m << "\n";
        CHECK(m);
        CHECK(mm->model_from_str("A B C"));
        CHECK_THROWS(mm->model_from_str("B A C"));
    }
    CHECK(text1.str() == text2.str()); // no difference between with or without '|'
}
