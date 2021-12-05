#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/metamodel.h"
#include "textx/scoping.h"

TEST_CASE("model_fqn_scope_provider", "[textx/metamodel]")
{
    {
        auto grammar1 = R"#(
            Model: packages+=Package;
            Package: 'package' name=ID
                'begin'
                    ( packages=Package | items=Item )*
                'end';
            Item: 'item' name=ID;
        )#";

        {
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

            CHECK( textx::scoping::dot_separated_name_search(m->val().obj(),"p1.C") == m->val()["packages"][0]["items"][0].obj() );
        }
    }
}
