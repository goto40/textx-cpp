#include "catch.hpp"
#include "textx/metamodel.h"
#include <iostream>
#include <sstream>
#include <filesystem>

TEST_CASE("from_python/examples/StateMachine 1", "[textx/from_python/examples]")
{
    auto p_grammar = std::filesystem::path(__FILE__).parent_path().append("state_machine.tx");
    auto mm = textx::metamodel_from_file(p_grammar);

    auto p_model1 = std::filesystem::path(__FILE__).parent_path().append("miss_grant_controller.sm");
    auto m1 = mm->model_from_file(p_model1);

    CHECK( (*m1)["events"].size()==5 );
    CHECK( (*m1)["resetEvents"].size()==1 );
    CHECK( (*m1)["commands"].size()==4 );
    CHECK( (*m1)["states"].size()==5 );

    auto p_model2 = std::filesystem::path(__FILE__).parent_path().append("gate.sm");
    auto m2 = mm->model_from_file(p_model2);

    CHECK( (*m2)["events"].size()==2 );
    CHECK( (*m2)["resetEvents"].size()==0 );
    CHECK( (*m2)["commands"].size()==2 );
    CHECK( (*m2)["states"].size()==2 );
}
