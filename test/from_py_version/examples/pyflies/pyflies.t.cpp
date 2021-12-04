#include "catch.hpp"
#include "textx/metamodel.h"
#include <iostream>
#include <sstream>
#include <filesystem>

TEST_CASE("from_python/examples/pyflies1", "[textx/from_python/examples]")
{
    auto p_grammar = std::filesystem::path(__FILE__).parent_path().append("pyflies.tx");
    auto mm = textx::metamodel_from_file(p_grammar);

    auto p_model1 = std::filesystem::path(__FILE__).parent_path().append("experiment.pf");
    auto m1 = mm->model_from_file(p_model1);

    auto p_model2 = std::filesystem::path(__FILE__).parent_path().append("order_changed_of_optional_element_in_unordered_group.pf");
    auto m2 = mm->model_from_file(p_model2);
}
