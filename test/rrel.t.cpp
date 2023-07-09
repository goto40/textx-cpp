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
 
    textx::arpeggio::ParserResult m;
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
    r = textx::rrel::create_RREL_expression("a.'b'~b.'c'~x");
    CHECK(r->str() == "a.'b'~b.'c'~x");
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

        Comment: /#.*?$/;
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

    P2_tst = textx::rrel::find(rec, "", "..(.).(..)");
    CHECK( P2_tst == P2 ); // p!

    P2_tst = textx::rrel::find(rec, "", "..((.)*)*.(..)");
    CHECK( P2_tst == P2 );

    auto none = textx::rrel::find(m->val().obj(), "", "..");
    CHECK( none == nullptr );

    auto mobj = textx::rrel::find(m->val().obj(), "", ".");  // '.' references the current element
    CHECK(mobj == m->val().obj());

    auto inner = textx::rrel::find(m->val().obj(), "inner", "~packages.~packages.~classes.attributes");
    CHECK((*inner)["name"].str() == "inner");

    auto package_Inner = textx::rrel::find(inner, "Inner", "parent(OBJECT)*.packages");
    REQUIRE( package_Inner != nullptr );
    CHECK( package_Inner->is_instance("Package") );
    CHECK( !package_Inner->is_instance("Class") );

    CHECK( nullptr == textx::rrel::find(inner, "P2", "parent(Class)*.packages") );

    // expensive version of a "Plain Name" scope provider:
    auto other_inner = textx::rrel::find(m->val().obj(), "inner", "~packages*.~classes.attributes");
    CHECK( inner == other_inner );

    auto rec2 = textx::rrel::find(m->val().obj(), "P2.Part2.rec", "other1,other2,packages*.classes.attributes");
    CHECK( rec2 == rec );

    rec2 = textx::rrel::find(m->val().obj(), "P2.Part2.rec", "other1,packages*.classes.attributes,other2");
    CHECK( rec2 == rec );

    rec2 = textx::rrel::find(m->val().obj(), "P2::Part2::rec", "other1,packages*.classes.attributes,other2","", "::");
    CHECK( rec2 == rec );

    rec2 = textx::rrel::find(m->val().obj(), "P2.Part2.rec", "other1,other2,other3");
    CHECK( rec2 == nullptr );

    rec2 = textx::rrel::find(m->val().obj(), "P2.Part2.rec", "(packages,classes,attributes)*");
    CHECK( rec2 == rec );

    rec2 = textx::rrel::find(m->val().obj(), "P2.Part2.rec", "(packages,(classes,attributes)*)*.attributes");
    CHECK( rec2 == rec );

    rec2 = textx::rrel::find(m->val().obj(), "rec", "(~packages,~classes,attributes,classes)*");
    CHECK( rec2 == rec );

    rec2 = textx::rrel::find(m->val().obj(), "rec",
                "(~packages,~classes,attributes,classes)*", "OBJECT");
    CHECK( rec2 == rec );

    rec2 = textx::rrel::find(m->val().obj(), "rec",
                "(~packages,~classes,attributes,classes)*", "Attribute");
    CHECK( rec2 == rec );

    rec2 = textx::rrel::find(m->val().obj(), "rec",
                "(~packages,~classes,attributes,classes)*", "Package");
    CHECK( rec2 == nullptr );

    rec2 = textx::rrel::find(m->val().obj(), "rec",
                "(~packages,classes,attributes,~classes)*", "Class");
    CHECK((*rec2)["name"].str() == "rec");
    CHECK( rec2 != rec );

    rec2 = textx::rrel::find(m->val().obj(), "rec",
                "(~packages,~classes,attributes,classes)*", "Class");
    CHECK((*rec2)["name"].str() == "rec");
    CHECK( rec2 != rec );

    auto t = textx::rrel::find(m->val().obj(), "", ".");
    CHECK( t == m->val().obj() );

    t = textx::rrel::find(m->val().obj(), "", "(.)");
    CHECK( t == m->val().obj() );

    t = textx::rrel::find(m->val().obj(), "", "(.)*");
    CHECK( t == m->val().obj() );

    t = textx::rrel::find(m->val().obj(), "", "(.)*.no_existent"); // inifite recursion stopper
    CHECK( t == nullptr );

    rec2 = textx::rrel::find(m->val().obj(), "rec",
                "(.)*.(~packages,~classes,attributes,classes)*", "Class");
    CHECK((*rec2)["name"].str() == "rec");
    CHECK( rec2 != rec ); // it is the class...

    // Here, we test the start_from_root/start_locally logic:
    auto P2t = textx::rrel::find(rec, "P2", "(.)*.packages");
    CHECK( P2t == nullptr );
    P2t = textx::rrel::find(rec, "P2", "(.,not_existent_but_root)*.packages");
    CHECK( P2t == P2 );
    auto rect = textx::rrel::find(rec, "rec", "(~packages)*.(..).attributes");
    CHECK( rect == nullptr );
    rect = textx::rrel::find(rec, "rec", "(.,~packages)*.(..).attributes");
    CHECK( rect == rec );
}

// This is a basic extra test to demonstrate `()*`
// in RREL expressions.
TEST_CASE("adapted_from_python_test_rrel_repetitions", "[textx/rrel]")
{
    auto my_metamodel = textx::metamodel_from_str(R"#(
        Model: entries*=Entry;
        Entry: name=ID (':' ref=[Entry])?;
        Comment: /\/\/.*?$/;
    )#");

    auto my_model = my_metamodel->model_from_str(R"#(
        a: b
        c
        b: a
    )#");

    auto a = textx::rrel::find(my_model->val().obj(), "a", "entries.ref*");
    REQUIRE(a!=nullptr);
    CHECK( (*a)["name"].str() == "a" );
    auto b = textx::rrel::find(my_model->val().obj(), "b", "entries.ref*");
    REQUIRE(b!=nullptr);
    CHECK( (*b)["name"].str() == "b" );
    auto c = textx::rrel::find(my_model->val().obj(), "c", "entries.ref*");
    REQUIRE(c!=nullptr);
    CHECK( (*c)["name"].str() == "c" );

    auto a2 = textx::rrel::find(my_model->val().obj(), "a.b.a", "entries.ref*");
    CHECK( a2 == a );

    auto b2 = textx::rrel::find(my_model->val().obj(), "b.a.b", "entries.ref*");
    CHECK( b2 == b );

    {
        auto [res, objpath] = std::get<0>(textx::rrel::find_object_with_path(my_model->val().obj(), "b.a.b", "entries.ref*"));
        CHECK( res == b );
        REQUIRE( objpath.size() == 3 );
        CHECK( objpath[objpath.size()-1].lock() == res );
        CHECK( textx::rrel::build_fqn(objpath) == "b.a.b" );

        a2 = textx::rrel::find(my_model->val().obj(), "b.a.b.a", "entries.ref*");
        CHECK( a2 == a );
    }
    {
        auto [res, objpath] = std::get<0>(textx::rrel::find_object_with_path(my_model->val().obj(), "b.a.b.a", "entries.ref*"));
        CHECK( res == a );
        CHECK( objpath.size() == 4 );
        CHECK( objpath[objpath.size()-1].lock() == res );
        CHECK( textx::rrel::build_fqn(objpath,".") == "b.a.b.a" );
        CHECK( textx::rrel::build_fqn(objpath,"::") == "b::a::b::a" );

        a2 = textx::rrel::find(my_model->val().obj(), "b.a.b.a.b.a.b.a.b.a", "entries.ref*");
        CHECK( a2 == a );
        b2 = textx::rrel::find(my_model->val().obj(), "b.a.b.a.b.a.b.a.b.a.b", "entries.ref*");
        CHECK( b2 == my_model->fqn("b") );
    }
}

TEST_CASE("rrel_scope_provider", "[textx/rrel]")
{
    auto mm = textx::metamodel_from_str(R"#(
        Model: a+=A r+=R;
        A: 'A' name=ID '{' a*=A  '}';
        R: 'R' a+=[A|FQN][','];
        FQN: ID ('.' ID)*;
    )#");
    mm->set_resolver("R.a", std::make_unique<textx::rrel::RRELScopeProvider>("+p:a*"));
    auto m = mm->model_from_str(R"#(
        A a1 {
            A aa1 {
                A aaa1 {}
                A aab1 {}
            }
        }
        A a2 {
            A aa2 {}
        }
        A R {
            A r2 {}
        }
        R a1.aa1.aaa1, a1.aa1.aab1, R, R.r2
        R R
        R a2.aa2
    )#");    
    CHECK( m->val()["r"].size() == 3 );
    CHECK( m->val()["r"][0]["a"][0].obj() == m->fqn("a1.aa1.aaa1") );
    CHECK( m->val()["r"][0]["a"][1].obj() == m->fqn("a1.aa1.aab1") );
    CHECK( m->val()["r"][0]["a"][2].obj() == m->fqn("R") );
    CHECK( m->val()["r"][0]["a"][3].obj() == m->fqn("R.r2") );
    CHECK( m->val()["r"][1]["a"][0].obj() == m->fqn("R") );
    CHECK( m->val()["r"][2]["a"][0].obj() == m->fqn("a2.aa2") );
}

TEST_CASE("from_python_tests_components1", "[textx/rrel]")
{
    auto p_grammar = std::filesystem::path(__FILE__).parent_path().append("rrel/components/ComponentsRrel.tx");
    auto mm = textx::metamodel_from_file(p_grammar);
    for(auto fn : {
        "example.components",
        "example_A.components",
        "example_B.components",
        "example_inherit1.components",
        "example_inherit2.components",
        "example_inherit3.components"
    }) {
        auto m = mm->model_from_file(std::filesystem::path(__FILE__).parent_path().append("rrel/components").append(fn));
        CHECK(m!=nullptr);
    }

    {
        auto m = mm->model_from_file(std::filesystem::path(__FILE__).parent_path().append("rrel/components").append("example_inherit1.components"));
        CHECK(m!=nullptr);
        auto usage = m->fqn("usage");
        auto connect3 = (*usage)["connections"][2].obj();
        CHECK( (*connect3)["from_port"].obj() == m->fqn("base.Start.output1"));
    }

    for(auto fn : {
        "example_err1.components",
        "example_err2.components"
    }) {
        CHECK_THROWS( (void)mm->model_from_file(std::filesystem::path(__FILE__).parent_path().append("rrel/components").append(fn)) );
    }
}

TEST_CASE("test_rrel_multifile", "[textx/rrel]")
{
    auto p_grammar = std::filesystem::path(__FILE__).parent_path().append("rrel/example0/Grammar.tx");
    auto mm = textx::metamodel_from_file(p_grammar);
    auto m = mm->model_from_file(std::filesystem::path(__FILE__).parent_path().append("rrel/example0/main.model"));
    CHECK(m!=nullptr);
}

TEST_CASE("test_rrel_multifile_nav_special_case", "[textx/rrel]")
{
    //TODO check this in real/python textx version
    auto p_grammar = std::filesystem::path(__FILE__).parent_path().append("rrel/example0/Grammar.tx");
    auto mm = textx::metamodel_from_file(p_grammar);
    auto m0 = mm->model_from_file(std::filesystem::path(__FILE__).parent_path().append("rrel/example0/navigation0.model"));
    CHECK(m0!=nullptr);
    auto m1 = mm->model_from_file(std::filesystem::path(__FILE__).parent_path().append("rrel/example0/navigation1.model"));
    CHECK(m1!=nullptr);
    REQUIRE_THROWS_WITH( (void)mm->model_from_file(std::filesystem::path(__FILE__).parent_path().append("rrel/example0/navigation1_err.model")),
        Catch::Matchers::Contains( "'a2' not found" )
    );
}

TEST_CASE("from_python_tests_test_split_str_multifile", "[textx/rrel]")
{
    auto p_grammar = std::filesystem::path(__FILE__).parent_path().append("rrel/example1/Grammar.tx");
    auto mm = textx::metamodel_from_file(p_grammar);
    CHECK((*mm)["FQN"].tx_params().size()==1);
    REQUIRE((*mm)["FQN"].tx_params().count("split")==1);
    CHECK((*mm)["FQN"].tx_params("split") == "::");
    auto m = mm->model_from_file(std::filesystem::path(__FILE__).parent_path().append("rrel/example1/main.model"));
    CHECK(m!=nullptr);
}

TEST_CASE("rrel_regression1", "[textx/rrel]")
{
    auto mm = textx::metamodel_from_str(R"#(
        Model:
            cls*=Cls
            obj*=Obj
            call*=Call
        ;
        Cls: "class" name=ID (
            ( "extends" extends+=[Cls][','])?
            "{"
                methods*=Method
            "}"
            )?;

        Method: "method" name=ID;
        Obj: "obj" name=ID ":" ref=[Cls];
        Call: "call" name=ID ":" obj=[Obj] "." method=[Method|ID|.~obj.~ref.~extends*.methods];

        Comment: /\/\/.*?$/;
    )#");
    auto m = mm->model_from_str(R"#(
        class A extends B,C {
        }
        class B extends D {
            method b
        }
        class C extends D {
            method c
        }
        class D {
            method d
        }
        obj a:A
        call callb: a.b
        call callc: a.c
        call calld: a.d
    )#");
    CHECK(m!=nullptr);
}

TEST_CASE("test_rrel_navigation_with_fixed_str", "[textx/rrel]")
{
    auto mm = textx::metamodel_from_str(R"#(
        Model: types_collection*=TypesCollection ('activeTypes' '=' active_types=[TypesCollection])? usings*=Using;
        Using: 'using' name=ID "=" type=[Type|ID|+m:
                ~active_types.types,                // "regular lookup"
                'builtin'~types_collection.types    // "default lookup" - name "builtin" hard coded in grammar
            ];
        TypesCollection: 'types' name=ID "{" types*=Type "}";
        Type: 'type' name=ID;
        Comment: /#.*?$/;
    )#");
    auto builtin = mm->model_from_str(R"#(
        types builtin {
            type i32
            type i64
            type f32
            type f64
        }
    )#");
    mm->add_builtin_model(builtin);

    auto m = mm->model_from_str(R"#(
        types MyTypes {
            type Int
            type Double
        }
        types OtherTypes {
            type Foo
            type Bar
        }
        activeTypes=MyTypes
        using myDouble = Double
        using myInt = Int    # found via "regular lookup"
        using myi32 = i32    # found via "default lookup"
        # using myFoo = Foo  # --> not found 
    )#");
}
