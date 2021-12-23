#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/lang.h"
#include "textx/arpeggio.h"
#include "textx/rrel.h"
#include "textx/metamodel.h"

TEST_CASE("adapted_from_python_textx_tests_test_rrel_basic_parser1", "[textx/rrel]")
{
    textx::lang::TextxGrammar parser;
    parser.add_rule("rrel_expression_standalone", 
                    textx::arpeggio::sequence({
                        parser.ref("rrel_expression"),
                        textx::arpeggio::end_of_file()
                    }));
    parser.set_main_rule("rrel_expression_standalone");
 
    std::optional<textx::arpeggio::Match> m;
    m = parser.parse_or_throw("^pkg*.cls");
    m = parser.parse_or_throw("obj.ref.~extension *.methods");
    m = parser.parse_or_throw("instance.(type.vals)*");
    m = parser.parse_or_throw("+mp:a.b.c");
    CHECK_THROWS(parser.parse_or_throw("+U:a.b.c")); // U not allowed
    CHECK_THROWS(parser.parse_or_throw("a,b,c,")); // "," at the end not allowed
    CHECK_THROWS(parser.parse_or_throw("")); // empty seq. not allowed
}

TEST_CASE("adapted_from_python_textx_tests_test_rrel_basic_parser2", "[textx/rrel]")
{
    std::unique_ptr<textx::rrel::RRELBase> r;
    r = textx::rrel::create_RREL_expression("^pkg*.cls");
    CHECK(r->str() == "(..)*.(pkg)*.cls");
    r = textx::rrel::create_RREL_expression("obj.ref.~extension *.methods");
    CHECK(r->str() == "obj.ref.(~extension)*.methods");
    r = textx::rrel::create_RREL_expression("type.vals");
    CHECK(r->str() == "type.vals");
    r = textx::rrel::create_RREL_expression("(type.vals)");
    CHECK(r->str() == "(type.vals)");
    r = textx::rrel::create_RREL_expression("(type.vals)*");
    CHECK(r->str() == "(type.vals)*");
    r = textx::rrel::create_RREL_expression("instance . ( type.vals ) *");
    CHECK(r->str() == "instance.(type.vals)*");
    r = textx::rrel::create_RREL_expression("a,b,c");
    CHECK(r->str() == "a,b,c");
    r = textx::rrel::create_RREL_expression("a.b.c");
    CHECK(r->str() == "a.b.c");
    r = textx::rrel::create_RREL_expression("parent(NAME)");
    CHECK(r->str() == "parent(NAME)");
    r = textx::rrel::create_RREL_expression("+p:a.b.c");
    CHECK(r->str() == "+p:a.b.c");
    r = textx::rrel::create_RREL_expression("+mp:a.b.c");
    CHECK(r->str() == "+mp:a.b.c");
}

TEST_CASE("simple_rrel1", "[textx/rrel]")
{
    auto mm = textx::metamodel_from_str(R"#(
        Model: packages+=Package;
        Package: "package" name=ID "{" packages*=Package objects*=Object "}";
        Object: "object" name=ID;
    )#");
    auto m = mm->model_from_str(R"(
        package a {
            package b {
                object c
            }
        }
    )");    
    auto res = textx::rrel::find_object_with_path(m->val().obj(), "a.b.c", "packages.packages.objects");
    CHECK( std::get<0>(res).obj == m->fqn("a.b.c") );

    res = textx::rrel::find_object_with_path(m->val().obj(), "a.b.c.d", "packages.packages.objects");
    CHECK( std::get<0>(res).obj == nullptr );

    res = textx::rrel::find_object_with_path(m->val().obj(), "a.b", "packages.packages.objects");
    CHECK( std::get<0>(res).obj == nullptr );

    // same with *

    res = textx::rrel::find_object_with_path(m->val().obj(), "a.b.c", "packages*.objects");
    CHECK( std::get<0>(res).obj == m->fqn("a.b.c") );

    res = textx::rrel::find_object_with_path(m->val().obj(), "a.b.c.d", "packages*.objects");
    CHECK( std::get<0>(res).obj == nullptr );

    res = textx::rrel::find_object_with_path(m->val().obj(), "a.b", "packages*.objects");
    CHECK( std::get<0>(res).obj == nullptr );
}

namespace {
    const char* metamodel_str = R"#(
        Model:
            packages*=Package
        ;

        Package:
            'package' name=ID '{'
            packages*=Package
            classes*=Class
            '}'
        ;

        Class:
            'class' name=ID '{'
                attributes*=Attribute
            '}'
        ;

        Attribute:
                'attr' name=ID ';'
        ;

        Comment: /#.*/;
        FQN: ID('.'ID)*;
    )#";

    const char* modeltext=modeltext = R"#(
        package P1 {
            class Part1 {
            }
        }
        package P2 {
            package Inner {
                class Inner {
                    attr inner;
                }
            }
            class Part2 {
                attr rec;
            }
            class C2 {
                attr p1;
                attr p2a;
                attr p2b;
            }
            class rec {
                attr p1;
            }
        }
    )#";
}

TEST_CASE("adapted_from_python_test_rrel_basic_lookup", "[textx/rrel]")
{
    auto mm = textx::metamodel_from_str(metamodel_str);
    auto m = mm->model_from_str(modeltext);

    auto P2 = textx::rrel::find(m->val().obj(), "P2", "packages");
    CHECK((*P2)["name"].str() == "P2");
    auto Part2 = textx::rrel::find(m->val().obj(), "P2.Part2", "packages.classes");
    CHECK((*Part2)["name"].str() == "Part2");

    auto rec = textx::rrel::find(m->val().obj(), "P2.Part2.rec", "packages.classes.attributes");
    CHECK((*rec)["name"].str() == "rec");
    CHECK((*rec).parent() == Part2);

    auto other_P2 = textx::rrel::find(m->val().obj(), "P2", "(packages)");
    CHECK( other_P2 == P2 );

    CHECK( m->val().obj()->tx_model() == m );

    auto other2_P2 = textx::rrel::find(m->val().obj(), "P2", "packages*");
    CHECK( other2_P2 == P2 );
    auto other_Part2 = textx::rrel::find(m->val().obj(), "P2.Part2", "packages*.classes");
    CHECK( other_Part2 == Part2 );
    auto other_rec = textx::rrel::find(m->val().obj(), "P2.Part2.rec", "packages*.classes.attributes");
    CHECK( other_rec == rec );
    CHECK( other_rec->parent() == Part2 );

    auto Part2_tst = textx::rrel::find(rec, "", "..");
    CHECK( Part2_tst == Part2 );

    auto P2_from_inner_node = textx::rrel::find(rec, "P2", "(packages)");
    CHECK( P2_from_inner_node == P2 );

    auto P2_tst = textx::rrel::find(rec, "", "parent(Package)");
    CHECK( P2_tst == P2 );

    P2_tst = textx::rrel::find(rec, "", "...");
    CHECK( P2_tst == P2 );

    P2_tst = textx::rrel::find(rec, "", ".(..).(..)");
    CHECK( P2_tst == P2 );

    P2_tst = textx::rrel::find(rec, "", "(..).(..)");
    CHECK( P2_tst == P2 );

    P2_tst = textx::rrel::find(rec, "", "...(.).(.)");
    CHECK( P2_tst == P2 );

    //P2_tst = textx::rrel::find(rec, "", "..(.).(..)");
    //CHECK( P2_tst == P2 ); // p!

    //P2_tst = textx::rrel::find(rec, "", "..((.)*)*.(..)");
    //CHECK( P2_tst == P2 );

    //auto none = textx::rrel::find(m->val().obj(), "", "..");
    //CHECK( none == nullptr );
/*

    m = textx::rrel::find(m->val().obj(), "", ".")  # '.' references the current element
    CHECK((*m is my_model

    inner = textx::rrel::find(m->val().obj(), "inner", "~packages.~packages.~classes.attributes")
    CHECK((*inner)["name"].str() == "inner"

    package_Inner = textx::rrel::find(inner, "Inner", "parent(OBJECT)*.packages")
    CHECK((*textx_isinstance(package_Inner, my_metamodel["Package"])
    CHECK((*not textx_isinstance(package_Inner, my_metamodel["Class"])

    CHECK((*None is find(inner, "P2", "parent(Class)*.packages")

    # expensive version of a "Plain Name" scope provider:
    inner = textx::rrel::find(m->val().obj(), "inner", "~packages*.~classes.attributes")
    CHECK((*inner)["name"].str() == "inner"

    rec2 = textx::rrel::find(m->val().obj(), "P2.Part2.rec", "other1,other2,packages*.classes.attributes")
    CHECK((*rec2 is rec

    rec2 = textx::rrel::find(m->val().obj(), "P2.Part2.rec", "other1,packages*.classes.attributes,other2")
    CHECK((*rec2 is rec

    rec2 = textx::rrel::find(m->val().obj(), "P2::Part2::rec", "other1,packages*.classes.attributes,other2",
                split_string="::")
    CHECK((*rec2 is rec

    rec2 = textx::rrel::find(m->val().obj(), "P2.Part2.rec", "other1,other2,other3")
    CHECK((*rec2 is None

    rec2 = textx::rrel::find(m->val().obj(), "P2.Part2.rec", "(packages,classes,attributes)*")
    CHECK((*rec2 is rec

    rec2 = textx::rrel::find(m->val().obj(), "P2.Part2.rec", "(packages,(classes,attributes)*)*.attributes")
    CHECK((*rec2 is rec

    rec2 = textx::rrel::find(m->val().obj(), "rec", "(~packages,~classes,attributes,classes)*")
    CHECK((*rec2)["name"].str() == "rec"

    rec2 = textx::rrel::find(m->val().obj(), "rec",
                "(~packages,~classes,attributes,classes)*", my_metamodel["OBJECT"])
    CHECK((*rec2)["name"].str() == "rec"

    rec2 = textx::rrel::find(m->val().obj(), "rec",
                "(~packages,~classes,attributes,classes)*", my_metamodel["Attribute"])
    CHECK((*rec2 is rec

    rec2 = textx::rrel::find(m->val().obj(), "rec",
                "(~packages,~classes,attributes,classes)*", my_metamodel["Package"])
    CHECK((*rec2 is None

    rec2 = textx::rrel::find(m->val().obj(), "rec",
                "(~packages,classes,attributes,~classes)*", my_metamodel["Class"])
    CHECK((*rec2)["name"].str() == "rec"
    CHECK((*rec2 is not rec  # it is the class...

    rec2 = textx::rrel::find(m->val().obj(), "rec",
                "(~packages,~classes,attributes,classes)*", my_metamodel["Class"])
    CHECK((*rec2)["name"].str() == "rec"
    CHECK((*rec2 is not rec  # it is the class...

    t = textx::rrel::find(m->val().obj(), "", ".")
    CHECK((*t is my_model

    t = textx::rrel::find(m->val().obj(), "", "(.)")
    CHECK((*t is my_model

    t = textx::rrel::find(m->val().obj(), "", "(.)*")
    CHECK((*t is my_model

    t = textx::rrel::find(m->val().obj(), "", "(.)*.no_existent")  # inifite recursion stopper
    CHECK((*t is None

    rec2 = textx::rrel::find(m->val().obj(), "rec",
                "(.)*.(~packages,~classes,attributes,classes)*", my_metamodel["Class"])
    CHECK((*rec2)["name"].str() == "rec"
    CHECK((*rec2 is not rec  # it is the class...

    # Here, we test the start_from_root/start_locally logic:
    P2t = textx::rrel::find(rec, "P2", "(.)*.packages")
    CHECK((*P2t is None
    P2t = textx::rrel::find(rec, "P2", "(.,not_existent_but_root)*.packages")
    CHECK((*P2t is P2
    rect = textx::rrel::find(rec, "rec", "(~packages)*.(..).attributes")
    CHECK((*rect is None
    rect = textx::rrel::find(rec, "rec", "(.,~packages)*.(..).attributes")
    CHECK((*rect is rec
    */
}
