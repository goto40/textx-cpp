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