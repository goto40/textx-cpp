#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/metamodel.h"

TEST_CASE("big_model_with_benchmark", "[textx/metamodel]")
{
    auto mm = textx::metamodel_from_str(R"#(
        Model: classes+=Class;
        Class: "class" name=ID "{" attrs+=Attribute "{" refs+=Ref "}" "}";
        Attribute: name=ID ":" class=[Class];
        Ref: "ref" ref=[Attribute|FQN|+m:..attrs.(~class.attrs)*];
        Comment: /#.*?$/;
        FQN: ID('.'ID)*;
    )#");

    auto create_model =[](size_t n){
        std::ostringstream model_text;
        for (size_t i=0;i<n;i++) {
            model_text << "class A" << i << " {\n";
            model_text << "  a: A" << i << "\n";
            model_text << "  b: A" << ((i+1)%n) << "\n";
            model_text << "  c: A" << ((i+n-1)%n) << "\n";
            model_text << "  d: A" << ((i+n-2)%n) << "\n";
            model_text << "  {\n";
            model_text << "  ref c\n";
            model_text << "  ref a.a.a.a\n";
            model_text << "  ref b.b.b\n";
            model_text << "  ref c.c.c\n";
            model_text << "  ref a.b.c.d\n";
            model_text << "  ref b.b.b.b\n";
            model_text << "  ref d.d.d.d\n";
            model_text << "  }\n";
            model_text << "}\n";
        }
        return model_text.str();
    };

    {
        auto m = mm->model_from_str(create_model(100));
        CHECK( (*m)["classes"].size() == 100 );
        CHECK( (*(*m)["classes"][0]["refs"][0]["ref"].obj())["name"].str() == "c" );
        CHECK( (*(*m)["classes"][0]["refs"][0]["ref"]["class"].obj())["name"].str() == "A99" );
        CHECK( (*(*m)["classes"][0]["refs"][1]["ref"]["class"].obj())["name"].str() == "A0" ); // a.a.a.a
        CHECK( (*(*m)["classes"][0]["refs"][2]["ref"]["class"].obj())["name"].str() == "A3" ); // b.b.b
        CHECK( (*(*m)["classes"][0]["refs"][3]["ref"]["class"].obj())["name"].str() == "A97" ); // c.c.c
    }

    BENCHMARK("big_model 50") {
        return mm->model_from_str(create_model(50));
    };
    BENCHMARK("big_model 100") {
        return mm->model_from_str(create_model(100));
    };
    // BENCHMARK("big_model 200") {
    //     return mm->model_from_str(create_model(200));
    // };
    // BENCHMARK("big_model 400") {
    //     return mm->model_from_str(create_model(400));
    // };
}
