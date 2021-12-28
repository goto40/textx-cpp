#include "catch.hpp"
#include "textx/metamodel.h"
#include "textx/workspace.h"

TEST_CASE("metamodel_importing_other_metamodels", "[textx/metamodel]")
{
    auto workspace = textx::Workspace::create();
    auto mm_fn = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/A.tx");
    auto mm = textx::metamodel_from_file(mm_fn);
}