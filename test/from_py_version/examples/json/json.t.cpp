#include "catch.hpp"
#include "textx/metamodel.h"
#include <iostream>
#include <sstream>
#include <filesystem>

TEST_CASE("from_python/examples/json1", "[textx/from_python/examples]")
{
    auto p_grammar = std::filesystem::path(__FILE__).parent_path().append("json.tx");
    auto mm = textx::metamodel_from_file(p_grammar);

    std::tuple<std::string, std::string> cfgs[]={
        {"example1.json","A meta-markup language, used to create markup languages such as DocBook"},
        {"example2.json","CloseDoc()"},
        {"example3.json","Images/Sun.png"},
        {"example4.json","LOCALHOST:1433;DatabaseName=goon"},
        {"example5.json","About Adobe CVG Viewer..."},
    };

    using Catch::Matchers::Contains;
    for (auto cfg : cfgs) {
        auto p_model = std::filesystem::path(__FILE__).parent_path().append(std::get<0>(cfg));
        auto m = mm->model_from_file(p_model);
        std::ostringstream s;
        s << m->val();
        //std::cout << s.str() << "\n";
        CHECK_THAT( s.str(), Contains( std::get<1>(cfg) ));
    }
}
