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
            CHECK((*m1)["x"].i() == 123);
            CHECK((*m2)["x"].str() == "123"); // string is decoded.
        }
    }
}

TEST_CASE("model_simple3", "[textx/metamodel]")
{
    {
        auto mm = textx::metamodel_from_str(R"#(
            Model: points+=Point[','];
            Point: "(" x=NUMBER "," y=NUMBER ")";
        )#");
        auto m1 = mm->model_from_str("(1,2), (3,4.5)");
        CHECK(std::get<std::shared_ptr<textx::object::Object>>((*m1)["points"][0].data)->type == "Point");
        CHECK((*m1)["points"].size() == 2);
        CHECK((*m1)["points"][0]["x"].i() == 1);
        CHECK((*m1)["points"][0]["y"].i() == 2);
        CHECK((*m1)["points"][1]["x"].i() == 3);
        CHECK((*m1)["points"][1]["y"].str() == "4.5");
        //TODO: check if int was correctly converted: 
        // CHECK_THROWS((*m1)["points"][1]["y"].i() == 4);
    }
}

TEST_CASE("model_abstract_rule1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"#(
            Model: shapes+=Shape[','];
            Shape: Point|ComplexShape;
            ComplexShape: Circle|Line;
            Point: 'Point' '(' x=NUMBER ',' y=NUMBER ')';
            Circle: 'Circle' '(' center=Point ',' r=NUMBER ')';
            Line: 'Line' '(' p1=Point ',' p2=Point ')';
        )#";

        {
            auto mm = textx::metamodel_from_str(grammar1);

            CHECK(mm->operator[]("ComplexShape").tx_inh_by().size()==2);
            CHECK(mm->operator[]("Shape").tx_inh_by().size()==4);
            CHECK(mm->operator[]("Point").tx_inh_by().size()==0);

            CHECK(mm->get_fqn_for_rule("Point") == "Point");

            CHECK(mm->is_instance("Point", "Shape"));
            CHECK(mm->is_instance("Point", "Point"));
            CHECK(mm->is_base_of("Shape","Point"));
            CHECK(mm->is_base_of("Point","Point"));
            CHECK(!mm->is_base_of("Point","Shape"));
            CHECK_THROWS(mm->is_base_of("Unknown","Point"));
            CHECK_THROWS(mm->is_base_of("Point","Unknown"));

            auto m = mm->model_from_str(R"(
                Point(1,2),
                Circle(Point(333,4.5),9),
                Line(Point(0,0),Point(1,1))
            )");
            
            CHECK( (*mm)["Model"]["shapes"].cardinality == textx::AttributeCardinality::list );
            CHECK( (*mm)["Model"]["shapes"].types.size() == 1 );
            CHECK( (*mm)["Model"]["shapes"].type.value() == "Shape" );
            CHECK( (*m)["shapes"].size() == 3 );
            CHECK( (*m)["shapes"][0].obj()->type == "Point" );
            CHECK( (*m)["shapes"][1].obj()->type == "Circle" );
            CHECK( (*m)["shapes"][1]["center"]["x"].i() == 333 );
            CHECK( (*m)["shapes"][2].obj()->type == "Line" );
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
                CHECK((*m)["a"].is_boolean());
                CHECK((*m)["a"].boolean());
                CHECK(!(*m)["b"].is_boolean());
            }
            {
                auto m = mm->model_from_str("B A");
                CHECK(m);
                CHECK((*m)["a"].is_boolean());
                CHECK((*m)["a"].boolean());
                CHECK(!(*m)["b"].is_boolean());
            }
            CHECK(mm->model_from_str("B"));
            CHECK(mm->model_from_str("A"));
            auto m = mm->model_from_str("");
            CHECK(m);
            CHECK((*m)["a"].is_boolean());
            CHECK(!(*m)["a"].boolean());
            CHECK(!(*m)["b"].is_boolean());
        }
    }
}

TEST_CASE("model_ordered_choice2", "[textx/model]")
{
    std::ostringstream text1, text2;
    {
        auto mm = textx::metamodel_from_str("Model: ('A'|('B' 'C'))#;");
        auto m = mm->model_from_str("B C A");
        text1 << m->val() << "\n";
        CHECK(m);
        CHECK(mm->model_from_str("A B C"));
        CHECK_THROWS(mm->model_from_str("B A C"));
    }
    {
        auto mm = textx::metamodel_from_str("Model: ('A' ('B' 'C'))#;");
        auto m = mm->model_from_str("B C A");
        text2 << m->val() << "\n";
        CHECK(m);
        CHECK(mm->model_from_str("A B C"));
        CHECK_THROWS(mm->model_from_str("B A C"));
    }
    CHECK(text1.str() == text2.str()); // no difference between with or without '|'
}

TEST_CASE("model_with_optional_parts", "[textx/model]")
{
    auto mm = textx::metamodel_from_str(R"#(
        Model: entries*=Entry;
        Entry: A|B; 
        A: 'A' name=ID (':' ref=[Entry])?;
        B: 'B' name=ID (':' refs+=[Entry][','])?;
        Comment: /\/\/.*?$/;
    )#");

    auto m = mm->model_from_str(R"#(
        A a0
        B b0

        A a1: b1
        A a2: b1
        B b1: a1,a2
    )#");

    CHECK( (*m->fqn("a0"))["ref"].is_null());
    CHECK( (*m->fqn("a0"))["ref"].obj() == nullptr );
    CHECK( (*m->fqn("a0"))["ref"].is_obj() );
    CHECK( !(*m->fqn("a0"))["ref"].is_ref() );  // empty refs are mapped to null-objs.
    CHECK( !(*m->fqn("a0"))["ref"].is_list() );

    CHECK( !(*m->fqn("b0"))["refs"].is_obj() );
    CHECK( (*m->fqn("b0"))["refs"].is_list() );
    CHECK( (*m->fqn("b0"))["refs"].size()==0 );

    CHECK( (*m->fqn("a1"))["ref"].obj() == m->fqn("b1") );
    CHECK( (*m->fqn("a1"))["ref"].is_obj() );
    CHECK( (*m->fqn("a1"))["ref"].is_ref() );
    CHECK( !(*m->fqn("a1"))["ref"].is_list() );

    CHECK( (*m->fqn("b1"))["refs"].size()==2);
    CHECK( (*m->fqn("b1"))["refs"][0].obj() == m->fqn("a1") );
    CHECK( (*m->fqn("b1"))["refs"][1].obj() == m->fqn("a2") );
    CHECK( (*m->fqn("b1"))["refs"].is_list() );
    CHECK( (*m->fqn("b1"))["refs"][0].is_obj() );
    CHECK( (*m->fqn("b1"))["refs"][0].is_ref() );
}


TEST_CASE("model_boolean_assignment_with_alternatives", "[textx/model]")
{
    auto grammar1 = R"(
        Model: 'value' ( a='A' | a=B | 'none');
        B: x='B';
    )";
    auto mm = textx::metamodel_from_str(grammar1);
    auto m1 = mm->model_from_str("value A");
    auto m2 = mm->model_from_str("value B");
    auto m3 = mm->model_from_str("value none");

    CHECK( (*m1)["a"].is_str() )  ;
    CHECK( (*m1)["a"].str() == "A" );  

    CHECK( (*m2)["a"].is_obj() )  ;
    CHECK( (*m2)["a"]["x"].str() == "B" );  
    CHECK( !(*m2)["a"].is_null() );

    CHECK( (*m3)["a"].is_str() );

    auto grammar2 = R"(
        Model: 'value' ( a='A' | a=B | a?='C');
        B: x='B';
    )";
    CHECK_THROWS_WITH(textx::metamodel_from_str(grammar2), Catch::Matchers::Contains("boolean assignments must be alone"));

}