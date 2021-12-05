#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/metamodel.h"
#include "textx/scoping.h"

TEST_CASE("model_fqn_scope_provider", "[textx/metamodel]")
{
    {
        auto v = textx::scoping::separate_name("a.b.c");
        CHECK(v.size()==3);
        CHECK(v[0]=="a");
        CHECK(v[1]=="b");
        CHECK(v[2]=="c");
    }
    {
        auto v = textx::scoping::separate_name("a");
        CHECK(v.size()==1);
        CHECK(v[0]=="a");
    }
    {
        CHECK_THROWS(textx::scoping::separate_name("a..b"));
        CHECK_THROWS(textx::scoping::separate_name("a.b."));
        CHECK_THROWS(textx::scoping::separate_name("."));
    }
    {
        auto grammar1 = R"#(
            Model: packages+=Package;
            Package: 'package' name=ID
                'begin'
                    ( packages=Package | items=Item )*
                'end';
            Item: 'item' name=ID;
        )#";

        auto modeltext = R"(
            package p1 begin
                item A
                item C
                package p2 begin
                    item A
                    item B
                end
            end
            package b1 begin
                package b2 begin
                    item X
                end
            end
        )";
        auto mm = textx::metamodel_from_str(grammar1);
        auto m = mm->model_from_str(modeltext);

        auto p1_C = textx::scoping::dot_separated_name_search(m->val().obj(),"p1.C");
        CHECK( p1_C == m->val()["packages"][0]["items"][1].obj() );
        auto p1_A = textx::scoping::dot_separated_name_search(p1_C->parent(),"A");
        CHECK( p1_A == m->val()["packages"][0]["items"][0].obj() );
        auto p1_p2_A = textx::scoping::dot_separated_name_search(p1_C->parent(),"p2.A");
        CHECK( p1_p2_A == m->val()["packages"][0]["packages"][0]["items"][0].obj() );
        auto none_C = textx::scoping::dot_separated_name_search(m->val().obj(),"C");
        CHECK( none_C == nullptr );
    }
}
