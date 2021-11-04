#include "catch.hpp"
#include "textx/metamodel.h"
#include "textx/workspace.h"

TEST_CASE("metamodel_importing_other_metamodels1", "[textx/multi_metamodel]")
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

TEST_CASE("metamodel_importing_other_metamodels_circular", "[textx/multi_metamodel]")
{
    auto workspace = textx::Workspace::create();
    auto mm_fn_A = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/A.tx");
    auto mm_fn_B = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/B.tx");
    auto mm_fn_C = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/C.tx");
    workspace->add_metamodel_from_file_for_extension(".a",mm_fn_A);
    workspace->add_metamodel_from_file_for_extension(".b",mm_fn_B);
    workspace->add_metamodel_from_file_for_extension(".c",mm_fn_C);

    {
        auto fnA = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/circular/model_a.a");
        auto mA = workspace->model_from_file(fnA);
        auto fnB = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/circular/model_b.b");
        auto mB = workspace->model_from_file(fnB);

        auto a_b = mA->fqn("a_b");
        CHECK( (*a_b)["ref"].obj() == mB->fqn("B") );
    }
}

TEST_CASE("metamodel_importing_other_metamodels_inheritance", "[textx/multi_metamodel]")
{
    auto workspace = textx::Workspace::create();
    auto mm_fn_A = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/A.tx");
    auto mm_fn_B = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/B.tx");
    auto mm_fn_C = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/C.tx");
    workspace->add_metamodel_from_file_for_extension(".a",mm_fn_A);
    workspace->add_metamodel_from_file_for_extension(".b",mm_fn_B);
    workspace->add_metamodel_from_file_for_extension(".c",mm_fn_C);

    {
        auto fnA = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/inheritance/model_a.a");
        auto mA = workspace->model_from_file(fnA);

        auto the_call_a1 = mA->fqn("the_call_a1");
        auto a1 = mA->fqn("A1.a1");
        CHECK( a1 == (*the_call_a1)["method"].obj() );
    }
}

TEST_CASE("metamodel_importing_other_metamodels_diamond", "[textx/multi_metamodel]")
{
    auto workspace = textx::Workspace::create();
    auto mm_fn_A = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/A.tx");
    auto mm_fn_B = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/B.tx");
    auto mm_fn_C = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/C.tx");
    workspace->add_metamodel_from_file_for_extension(".a",mm_fn_A);
    workspace->add_metamodel_from_file_for_extension(".b",mm_fn_B);
    workspace->add_metamodel_from_file_for_extension(".c",mm_fn_C);

    {
        auto fnA = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/metamodel_provider3/diamond/A_includes_B_C.a");
        auto mA = workspace->model_from_file(fnA);
    }
}

TEST_CASE("metamodel_referencing_other_metamodels", "[textx/multi_metamodel]")
{
    auto workspace = textx::Workspace::create();
    auto mm_fn_T = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/referenced_metamodel/Types.tx");
    auto mm_fn_D = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/referenced_metamodel/Data.tx");
    auto mm_fn_F = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/referenced_metamodel/Flow.tx");
    workspace->add_metamodel_from_file_for_extension(".etype",mm_fn_T);
    workspace->add_metamodel_from_file_for_extension(".edata",mm_fn_D);
    workspace->add_metamodel_from_file_for_extension(".eflow",mm_fn_F);

    {
        auto fn = std::filesystem::path(__FILE__).parent_path().append("multi_metamodel/referenced_metamodel/model/data_flow.eflow");
        auto m = workspace->model_from_file(fn);

        //TODO: test other files.
    }
}
