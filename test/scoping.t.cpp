#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/metamodel.h"
#include "textx/scoping.h"

TEST_CASE("model_ref1", "[textx/scoping]")
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

TEST_CASE("model_fqn_scope_provider", "[textx/scoping]")
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

TEST_CASE("model_ref_fqn", "[textx/scoping]")
{
    {
        auto grammar1 = R"#(
            Model: packages+=Package;
            Package: 'package' name=ID
                'begin'
                    ( packages=Package | items=Item | usings=Using )*
                'end';
            Item: 'item' name=ID;
            Using: 'using' name=ID '=' ref=[ItemOrPackage|FQN];
            FQN: ID ('.' ID)*;
            Comment: /\/\/.*?$/;
            ItemOrPackage: Item|Package;
        )#";

        {
            auto modeltext = R"(
                package p1 begin
                    item A
                    item C
                    package p2 begin
                        item A
                        item B
                        using p2_p2A=A
                        using p2_p1C=C
                        using p2_x=b1.b2.X
                        using b2r=b1.b2
                        //does not work with FQN, wait for REEL: using pass_through_ref=b2r.X
                    end
                    using p1_p1A=A
                    using p1_p1C=C
                    using p1_p2A=p2.A
                end
                package b1 begin
                    package b2 begin
                        item X
                    end
                end
            )";
            {
                auto mm = textx::metamodel_from_str(grammar1);
                CHECK_THROWS_WITH((void)mm->model_from_str(modeltext),Catch::Matchers::Contains("p2.A"));
                CHECK_THROWS_WITH((void)mm->model_from_str(modeltext),Catch::Matchers::Contains("b1.b2.X"));
            }
            {
                auto mm = textx::metamodel_from_str(grammar1);
                mm->set_resolver("*.*", std::make_unique<textx::scoping::FQNRefResolver>());
                auto m = mm->model_from_str(modeltext);

                CHECK( (*m->fqn("p1.p2.p2_x"))["ref"].obj() == m->fqn("b1.b2.X") );
            }
        }
    }
}

TEST_CASE("model_ref_fqn_with_builtin", "[textx/scoping]")
{
    auto grammar1 = R"#(
        Model: packages+=Package;
        Package: 'package' name=ID
            'begin'
                ( packages=Package | items=Item | usings=Using )*
            'end';
        Item: 'item' name=ID;
        Using: 'using' name=ID '=' ref=[ItemOrPackage|FQN];
        FQN: ID ('.' ID)*;
        Comment: /\/\/.*?$/;
        ItemOrPackage: Item|Package;
    )#";

    auto modeltext = R"(
        package p1 begin
            item A
            item C
            using X = p1.builtin.X
        end
    )";
    auto modeltext_builtin = R"(
        package p1 begin
            package builtin begin
                item X
            end
        end
    )";

    auto mm = textx::metamodel_from_str(grammar1);
    mm->set_resolver("*.*", std::make_unique<textx::scoping::FQNRefResolver>());
    mm->add_builtin_model(mm->model_from_str(modeltext_builtin));
    CHECK_NOTHROW(mm->model_from_str(modeltext));
}

TEST_CASE("model_ref_fqn_bad_target_type", "[textx/scoping]")
{
    {
        auto grammar1 = R"#(
            Model: packages+=Package;
            Package: 'package' name=ID
                'begin'
                    ( packages=Package | items=Item | usings=Using )*
                'end';
            Item: 'item' name=ID;
            Using: 'using' name=ID '=' ref=[Item|FQN];
            FQN: ID ('.' ID)*;
            Comment: /\/\/.*?$/;
        )#";

        {
            auto modeltext = R"(
                package p1 begin
                    item A
                    item C
                    package p2 begin
                        item A
                        item B
                        using p2_p2A=A
                        using p2_p1C=C
                        using p2_x=b1.b2.X
                        using b2r=b1.b2 // bad type: exception <-------- expected exception
                        //does not work with FQN, wait for REEL: using pass_through_ref=b2r.X
                    end
                    using p1_p1A=A
                    using p1_p1C=C
                    using p1_p2A=p2.A
                end
                package b1 begin
                    package b2 begin
                        item X
                    end
                end
            )";
            auto mm = textx::metamodel_from_str(grammar1);
            mm->set_resolver("*.*", std::make_unique<textx::scoping::FQNRefResolver>());
            CHECK_THROWS_WITH( (void)mm->model_from_str(modeltext), Catch::Matchers::Contains("'b2' has not expected type 'Item'"));
        }
    }
}

TEST_CASE("model_ref1_type_check", "[textx/scoping]")
{
    {
        auto grammar1 = R"#(
            Model: as+=A bs+=B 'ref' ref=[Item];
            Item: A|B;
            A: 'a' name=ID;
            B: 'b' name=ID;
        )#";

        {
            auto mm = textx::metamodel_from_str(grammar1);
            auto m1 = mm->model_from_str("a A1 a A2 b B1 ref B1");
            auto m2 = mm->model_from_str("a A1 a A2 b B1 ref A2");
        }
    }
    {
        auto grammar1 = R"#(
            Model: as+=A bs+=B 'ref' ref=[A];
            Item: A|B;
            A: 'a' name=ID;
            B: 'b' name=ID;
        )#";

        {
            auto mm = textx::metamodel_from_str(grammar1);
            CHECK_THROWS_WITH( mm->model_from_str("a A1 a A2 b B1 ref B1"), Catch::Matchers::Contains("'B1' has not expected type 'A'") );
            auto m2 = mm->model_from_str("a A1 a A2 b B1 ref A2");
        }
    }
}
