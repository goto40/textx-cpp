#include "catch.hpp"
#include "textx/metamodel.h"
#include <iostream>
#include <sstream>
#include <filesystem>

TEST_CASE("from_python/examples/Entity 1", "[textx/from_python/examples]")
{
    auto p_grammar = std::filesystem::path(__FILE__).parent_path().append("entity.tx");
    auto mm = textx::metamodel_from_file(p_grammar);

    std::cout << *mm << "\n";

    auto p_model1 = std::filesystem::path(__FILE__).parent_path().append("person.ent");
    auto m1 = mm->model_from_file(p_model1);

    CHECK( (*m1)["types"].size()==5 );
    CHECK( (*m1)["entities"].size()==1 );

}
