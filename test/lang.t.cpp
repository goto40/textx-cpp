#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/lang.h"

TEST_CASE("simple0", "[textx/lang]")
{
    struct MyGrammar : textx::Grammar<> { 
        MyGrammar() {
            namespace ta = textx::arpeggio;
            set_main_rule("textx_rule");
            add_rule("textx_rule", ta::sequence({ref("rule_name"),
                                    ta::str_match(":"),
                                    ref("textx_rule_body"),
                                    ta::str_match(";"),
                                    ta::end_of_file()}));
            add_rule("textx_rule_body", ta::regex_match(R"([^;]+)"));
            add_rule("rule_name", ref("ident"));
            add_rule("ident", ta::regex_match(R"(\w+)"));
        }
    };
    auto grammar0 = R"(
        Model: 'hello';
    )";

    MyGrammar g;
    CHECK(g.parse(grammar0).first);
}


TEST_CASE("simple1", "[textx/lang]")
{
    using namespace textx::arpeggio;
    using namespace textx;
    {
        auto grammar1 = R"(
            Model: 'hello';
        )";
        auto grammar1_error = R"(
            Model: 'hello'
        )";

        textx::lang::TextxGrammar textx_grammar;
        CHECK(textx_grammar.parse_or_throw(grammar1));
        CHECK(!textx_grammar.parse(grammar1_error).first);
    }
}

TEST_CASE("simple2", "[textx/lang]")
{
    {
        auto grammar1 = R"(
            Model: A|B; // some comments!
            A: 'A';
            B: 'B';
        )";

        textx::lang::TextxGrammar textx_grammar;
        CHECK(textx_grammar.parse_or_throw(grammar1));
    }
}

TEST_CASE("rrel1", "[textx/lang]")
{
    {
        namespace ta = textx::arpeggio;

        struct MyGrammar : textx::lang::TextxGrammar {
            MyGrammar() {
                add_rule("main",ta::sequence({ref("rrel_expression"), ta::end_of_file()}));
                set_main_rule( "main" );
            }
        } textx_grammar;
        CHECK(textx_grammar.parse_or_throw("a.b.c"));
        CHECK(textx_grammar.parse_or_throw(".."));
        CHECK(textx_grammar.parse_or_throw("(..)"));
        CHECK(textx_grammar.parse_or_throw("parent(Attribute).(..).attributes.(~type.attributes)*"));
        CHECK(textx_grammar.parse_or_throw("parent(Attribute).(..).attributes.(~type.attributes)*"));
        CHECK(textx_grammar.parse_or_throw("+mp:parent(Attribute).(..).attributes.(~type.attributes)*,parent(Attribute).~type.enum_entries"));
        CHECK(textx_grammar.parse_or_throw(R"(+mp:
            parent(Attribute).(..).attributes.(~type.attributes)*,
            parent(Attribute).~type.enum_entries
        )"));
        CHECK(textx_grammar.parse_or_throw(R"(+mp:
            parent(Attribute).(..).attributes.(~type.attributes)*,
            parent(Attribute).~type.enum_entries,
            parent(Constant).~type.enum_entries,
            parent(Constants).constant_entries,
            parent(Struct).constant_entries,
            parent(Attribute).~variant_selector.~ref.~type.enum_entries,
            ^(package,packages)*.constants.constant_entries,
            ^(package,packages)*.items.constant_entries,
            ^(package,packages)*.items.enum_entries           
        )"));

        CHECK(!textx_grammar.parse("a.b.c.").first);
        CHECK(!textx_grammar.parse("").first);
    }
}
