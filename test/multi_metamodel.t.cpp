#include "catch.hpp"
#include "textx/metamodel.h"
#include "textx/workspace.h"

TEST_CASE("metamodel_importing_other_metamodels", "[textx/metamodel]")
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