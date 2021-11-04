#include "catch.hpp"
#include "mgrep.h"
#include <textx/istrings.h>

TEST_CASE("mgrep_simple1", "[mgrep]")
{
    auto cmd = "Point({% model.x[0] %},{% model.y[0] %})";
    auto cmd_err = "Point({% model.x[1] %},{% model.y[0] %})";
    mgrep::MGrep grep(R"(Model: (('x' '=' x=NUMBER)|('y' '=' y=NUMBER)|/\S+/)*;)");
    CHECK( grep.parse_and_store("MyPoint with x=1 and y=2.1") );
    CHECK( grep.matches().size()==1 );
    CHECK( grep.matches()[0].line==1 );
    {
        auto &m = *(grep.matches()[0].model);
        CHECK( m["x"].size()==1 );
        CHECK( m["y"].size()==1 );
        CHECK( m["x"][0].str()=="1" );
        CHECK( m["y"][0].str()=="2.1" );

        auto res = textx::istrings::i(
            cmd,
            { {"model", m.val().obj()} }
        );

        CHECK_THROWS_WITH( textx::istrings::i(
                cmd_err,
                { {"model", m.val().obj()} }
            ),
            Catch::Matchers::Contains("index out of bounds")
        );

        CHECK( res == "Point(1,2.1)" );
    }

    CHECK_THROWS_WITH(
        grep.parse_and_transform("x=2 y=3"),
        Catch::Matchers::Contains("you need a transform command")
    );

    grep.set_transform_command(cmd);
    CHECK( grep.parse_and_transform("x=2 y=3").value() == "Point(2,3)" );
}