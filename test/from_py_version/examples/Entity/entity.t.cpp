#include "catch.hpp"
#include "textx/metamodel.h"
#include <iostream>
#include <sstream>
#include <filesystem>

TEST_CASE("from_python/examples/Entity 1", "[textx/from_python/examples]")
{
    auto p_grammar = std::filesystem::path(__FILE__).parent_path().append("entity.tx");
    auto mm = textx::metamodel_from_file(p_grammar);
    mm->add_builtin_model(mm->model_from_str(R"(
        type integer
        type string
        entity None {
            dummy : None
        }
    )"));

    //std::cout << *mm << "\n";

    auto p_model1 = std::filesystem::path(__FILE__).parent_path().append("person.ent");
    auto m1 = mm->model_from_file(p_model1);

    CHECK( (*m1)["types"].size()==0 );
    CHECK( (*m1)["entities"].size()==2 );
    CHECK( (*m1)["entities"][0]["name"].str()=="Person" );
    CHECK( (*m1)["entities"][1]["name"].str()=="Address" );

    CHECK( (*m1)["entities"][0]["properties"][1]["name"].str()=="address" );
    CHECK( (*m1)["entities"][0]["properties"][1]["type"].obj()==(*m1)["entities"][1].obj() );
}
