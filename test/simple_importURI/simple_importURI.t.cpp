#include "catch.hpp"
#include "textx/metamodel.h"
#include <iostream>
#include <sstream>
#include <filesystem>

TEST_CASE("simple_importURI", "[textx/simple_importURI]")
{
    auto p_grammar = std::filesystem::path(__FILE__).parent_path().append("metamodel.tx");
    auto mm = textx::metamodel_from_file(p_grammar);

    auto p_model1 = std::filesystem::path(__FILE__).parent_path().append("a.model");
    auto m1 = mm->model_from_file(p_model1);

    auto p_model2 = std::filesystem::path(__FILE__).parent_path().append("b.model");
    auto m2 = mm->model_from_file(p_model2);

    CHECK( (*m1)["defs"].size()==2 );
    CHECK( (*m1)["refs"].size()==1 );

    CHECK( (*m2)["defs"].size()==2 );
    CHECK( (*m2)["refs"].size()==0 );

    CHECK( m1->tx_imported_models().at(0).lock() == m2 );
    CHECK( (*m1)["refs"][0]["ref"].is_resolved() );
    CHECK( (*m1)["refs"][0]["ref"].obj() == m2->fqn("B2"));
}
