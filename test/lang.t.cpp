#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/lang.h"

TEST_CASE("simple0", "[textx/lang]")
{
    struct MyGrammar : textx::Grammar { 
        MyGrammar() {
            namespace ta = textx::arpeggio;
            set_main_rule(ref("textx_rule"));
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
    CHECK(g.parse(grammar0));
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
        CHECK(textx_grammar.parse(grammar1));
        CHECK(!textx_grammar.parse(grammar1_error));
    }
}

TEST_CASE("simple2", "[textx/lang]")
{
    {
        auto grammar1 = R"(
            Model: A|B;
            A: 'A';
            B: 'B';
        )";

        textx::lang::TextxGrammar textx_grammar;
        CHECK(textx_grammar.parse(grammar1));
    }
}
