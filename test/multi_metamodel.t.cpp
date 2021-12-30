#include "catch.hpp"
#include "textx/metamodel.h"
#include "textx/workspace.h"

TEST_CASE("metamodel_importing_other_metamodels1", "[textx/metamodel]")
{
    auto workspace = textx::Workspace::create();
    auto mm_fn = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/Simple.tx");
    auto mm = textx::metamodel_from_file(mm_fn);

    CHECK(mm->get_fqn_for_rule("Model") == "Simple.Model");
    CHECK(mm->get_fqn_for_rule("Import") == "SimpleBase.Import");
    CHECK(mm->get_fqn_for_rule("A") == "SimpleBaseBase.A");
    CHECK(mm->get_fqn_for_rule("SimpleBase.A") == "SimpleBaseBase.A");
    CHECK(mm->get_fqn_for_rule("Simple.A") == "SimpleBaseBase.A");
}

TEST_CASE("metamodel_importing_other_metamodels2", "[textx/metamodel]")
{
    auto workspace = textx::Workspace::create();
    auto mm_fn_A = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/A.tx");
    auto mm_fn_B = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/B.tx");
    auto mm_fn_C = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/B.tx");
    workspace->add_metamodel_for_extension(".a",mm_fn_A);
    workspace->add_metamodel_for_extension(".b",mm_fn_B);
    workspace->add_metamodel_for_extension(".c",mm_fn_C);

    {
        auto fn = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/circular/model_a.a");
        auto m = workspace->model_from_file(fn);
    }
}
