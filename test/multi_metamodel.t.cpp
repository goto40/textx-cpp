#include "catch.hpp"
#include "textx/metamodel.h"
#include "textx/workspace.h"

TEST_CASE("multi_metamodel_simple1", "[textx/multi_metamodel]")
{
    auto workspace = textx::Workspace::create();
    auto mm = workspace->metamodel_from_file();
}