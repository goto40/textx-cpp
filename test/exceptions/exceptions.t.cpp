#include "catch.hpp"
#include "textx/metamodel.h"
#include <iostream>
#include <sstream>
#include <filesystem>

TEST_CASE("exceptions1", "[textx/exceptions]")
{
    auto p_grammar = std::filesystem::path(__FILE__).parent_path().append("broken.tx");
    REQUIRE_THROWS_WITH( (void)textx::metamodel_from_file(p_grammar),
        Catch::Matchers::Contains( "expected" )
        && Catch::Matchers::Contains( "str_match,:" )
        && Catch::Matchers::Contains( "broken.tx:2" )
    );
}

TEST_CASE("exceptions2", "[textx/exceptions]")
{
    auto p_grammar = std::filesystem::path(__FILE__).parent_path().append("metamodel.tx");
    auto mm = textx::metamodel_from_file(p_grammar);

    auto p_model1 = std::filesystem::path(__FILE__).parent_path().append("broken.model");
    REQUIRE_THROWS_WITH( (void)mm->model_from_file(p_model1),
        Catch::Matchers::Contains( "broken.model:3:5" )
        && Catch::Matchers::Contains( ": ref 'B2' not found" )
    );
}

TEST_CASE("exceptions3", "[textx/exceptions]")
{
    auto p_grammar = std::filesystem::path(__FILE__).parent_path().append("metamodel.tx");
    auto mm = textx::metamodel_from_file(p_grammar);

    auto p_model1 = std::filesystem::path(__FILE__).parent_path().append("importer.model");
    REQUIRE_THROWS_WITH( (void)mm->model_from_file(p_model1),
        Catch::Matchers::Contains( "broken.model:3:5" )
        && Catch::Matchers::Contains( ": ref 'B2' not found" )
    );
}
