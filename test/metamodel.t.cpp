#include "catch.hpp"
#include <iostream>
#include <sstream>
#include "textx/metamodel.h"

TEST_CASE("metamodel_simple_expression1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'simple model'; // the model is just a fixed string!
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.parsetree_from_str("simple model"));
        CHECK_THROWS(mm.parsetree_from_str("no model"));
    }
}

TEST_CASE("metamodel_simple_expression2", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: "A" 'B'* 'C'+;
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.parsetree_from_str("ACCC"));
        CHECK(mm.parsetree_from_str("ABBBC"));
        CHECK_THROWS(mm.parsetree_from_str("A"));
    }
}

TEST_CASE("metamodel_simple_expression3", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: ("A"|'B')* 'C'+;
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.parsetree_from_str("ACCC"));
        CHECK(mm.parsetree_from_str("AABBBC"));
        CHECK(mm.parsetree_from_str("BAC"));
        CHECK(mm.parsetree_from_str("C"));
        CHECK_THROWS(mm.parsetree_from_str("ABBACA"));
        CHECK_THROWS(mm.parsetree_from_str(""));
    }
}

TEST_CASE("metamodel_simple_expression4", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: ("A" 'B' 'C')#;
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.parsetree_from_str("ABC"));
        CHECK(mm.parsetree_from_str("CAB"));
        CHECK(mm.parsetree_from_str("CBA"));
        CHECK(mm.parsetree_from_str("BAC"));
        CHECK_THROWS(mm.parsetree_from_str("AAB"));
        CHECK_THROWS(mm.parsetree_from_str("C"));
    }
}

TEST_CASE("metamodel_simple_expression5_regex", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: "[" /\w+/ "]";
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.parsetree_from_str("[hello]"));
        CHECK_THROWS(mm.parsetree_from_str("[hello world]"));
        CHECK_THROWS(mm.parsetree_from_str("[]"));
    }
}

TEST_CASE("metamodel_simple_expression6_neg_lookahead", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: "[" !'key' /\w+/ "]";
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.parsetree_from_str("[hello]"));
        CHECK_THROWS(mm.parsetree_from_str("[key]"));
    }
}

TEST_CASE("metamodel_simple_expression6_pos_lookahead", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: "[" &("a"|'b'|'extra') /\w+/ "]";
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.parsetree_from_str("[a]"));
        CHECK(mm.parsetree_from_str("[best]"));
        CHECK(mm.parsetree_from_str("[extra]"));
        CHECK(mm.parsetree_from_str("[extra123]"));
        CHECK_THROWS(mm.parsetree_from_str("[xyz]"));
    }
}

TEST_CASE("metamodel_simple_assignment1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'value' '=' value=/\w+/;
        )";

        textx::Metamodel mm{grammar1};
        CHECK_THROWS(mm.parsetree_from_str("value="));
        CHECK(mm.parsetree_from_str("value=Hello"));
        CHECK_THROWS(mm.parsetree_from_str("value=Hello World"));

        CHECK(mm["Model"]["value"].cardinality == textx::AttributeCardinality::scalar);
        //std::cout << mm << "\n";
    }
}

TEST_CASE("metamodel_simple_assignment1_repeated", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'value' '=' (value=/\w+/)*;
        )";

        textx::Metamodel mm{grammar1};
        CHECK(mm.parsetree_from_str("value="));
        CHECK(mm.parsetree_from_str("value=Hello"));
        CHECK(mm.parsetree_from_str("value=Hello World"));

        CHECK(mm["Model"]["value"].cardinality == textx::AttributeCardinality::list);
        //std::cout << mm << "\n";
    }
}

TEST_CASE("metamodel_simple_assignment2", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'value' '=' value+=/\w+/;
        )";

        textx::Metamodel mm{grammar1};
        CHECK_THROWS(mm.parsetree_from_str("value="));
        CHECK(mm.parsetree_from_str("value=Hello"));
        CHECK(mm.parsetree_from_str("value=Hello World"));

        //std::cout << mm << "\n";
        CHECK(mm["Model"]["value"].cardinality == textx::AttributeCardinality::list);
    }
}

TEST_CASE("metamodel_simple_assignment3", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'value' '=' value+=/\w+/[','];
        )";

        textx::Metamodel mm{grammar1};

        CHECK_THROWS(mm.parsetree_from_str("value="));
        CHECK(mm.parsetree_from_str("value=Hello"));
        CHECK(mm.parsetree_from_str("value=Hello, World"));
        CHECK_THROWS(mm.parsetree_from_str("value=Hello World"));

        CHECK(mm["Model"]["value"].cardinality == textx::AttributeCardinality::list);
    }
    {
        auto grammar1 = R"(
            Model: 'value' '=' value*=/\w+/[','];
        )";

        textx::Metamodel mm{grammar1};

        CHECK(mm.parsetree_from_str("value="));
        CHECK(mm.parsetree_from_str("value=Hello"));
        CHECK(mm.parsetree_from_str("value=Hello, World"));
        CHECK_THROWS(mm.parsetree_from_str("value=Hello World"));

        CHECK(mm["Model"]["value"].cardinality == textx::AttributeCardinality::list);
    }
}

TEST_CASE("metamodel_simple_unordered_group_test1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: ("A" 'B' 'C')#;
        )";

        textx::Metamodel mm{grammar1};

        CHECK_THROWS(mm.parsetree_from_str("AB"));
        CHECK(mm.parsetree_from_str("ABC"));
        CHECK(mm.parsetree_from_str("BCA"));
        CHECK(mm.parsetree_from_str("C B A"));
        CHECK_THROWS(mm.parsetree_from_str("A,B,C"));
    }
}

TEST_CASE("metamodel_simple_unordered_group_test2", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: ("A" 'B' 'C')#[','];
        )";

        textx::Metamodel mm{grammar1};
        
        CHECK_THROWS(mm.parsetree_from_str("A,B"));
        CHECK_THROWS(mm.parsetree_from_str("ABC"));
        CHECK(mm.parsetree_from_str("A,B,C"));
        CHECK(mm.parsetree_from_str("B,C,A"));
        CHECK(mm.parsetree_from_str("C,B,A"));
        CHECK_THROWS(mm.parsetree_from_str("A,B,"));
    }
}

TEST_CASE("metamodel_simple_unordered_group_test3", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: ("A" 'B' 'C')#[/\d*/];
        )";

        textx::Metamodel mm{grammar1};
        
        CHECK_THROWS(mm.parsetree_from_str("A1B"));
        CHECK(mm.parsetree_from_str("ABC"));
        CHECK(mm.parsetree_from_str("A1B2C"));
        CHECK(mm.parsetree_from_str("B3C123A"));
        CHECK(mm.parsetree_from_str("CB1A"));
        CHECK_THROWS(mm.parsetree_from_str("A1B2"));
    }
}

TEST_CASE("metamodel_simple_rule_ref", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: A B+ C;
            A: "a";
            B: 'b';
            C: 'c';
        )";

        textx::Metamodel mm{grammar1};
        
        CHECK_THROWS(mm.parsetree_from_str("ac"));
        CHECK(mm.parsetree_from_str("abc"));
        CHECK(mm.parsetree_from_str("abbc"));
    }
}

TEST_CASE("metamodel_simple_assignment_and_rule_ref1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'value' '=' value=MYID;
            MYID: /[^\d\W]\w*\b/;
        )";

        textx::Metamodel mm{grammar1};
        CHECK_THROWS(mm.parsetree_from_str("value="));
        CHECK(mm.parsetree_from_str("value=Hello123"));
        CHECK_THROWS(mm.parsetree_from_str("value=Hello World"));
        CHECK_THROWS(mm.parsetree_from_str("value=123Hello"));

        CHECK(mm["Model"]["value"].cardinality == textx::AttributeCardinality::scalar);
        CHECK(mm["MYID"].type() == textx::RuleType::match);
        //std::cout << mm << "\n";
    }
}

TEST_CASE("metamodel_simple_abstract_rule1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'value' '=' value=A;
            A: A1|A2;
            A1: x='a1';
            A2: 'a2';
        )";

        textx::Metamodel mm{grammar1};
        CHECK_THROWS(mm.parsetree_from_str("value="));
        CHECK(mm.parsetree_from_str("value=a1"));
        CHECK(mm.parsetree_from_str("value=a2"));

        CHECK(mm["Model"]["value"].cardinality == textx::AttributeCardinality::scalar);
        CHECK(mm["A1"].type() == textx::RuleType::common);
        CHECK(mm["A2"].type() == textx::RuleType::match);
        CHECK(mm["A"].type() == textx::RuleType::abstract);
        CHECK(mm["Model"]["value"].is_str() == false);
        CHECK(mm["Model"]["value"].type.value() == "A");
        //std::cout << mm << "\n";
    }
}

TEST_CASE("metamodel_simple_abstract_rule2", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'value' '=' (a=A1|a=A2) a1=A1 a2=A2;
            A: A1|A2;
            A1: x='a1';
            A2: x='a2';
        )";

        textx::Metamodel mm{grammar1};
        CHECK_THROWS(mm.parsetree_from_str("value="));
        CHECK(mm.parsetree_from_str("value=a1 a1 a2"));
        CHECK(mm.parsetree_from_str("value=a2 a1 a2"));

        CHECK(mm["Model"]["a"].cardinality == textx::AttributeCardinality::scalar);
        CHECK(mm["A1"].type() == textx::RuleType::common);
        CHECK(mm["A2"].type() == textx::RuleType::common);
        CHECK(mm["A"].type() == textx::RuleType::abstract);
        CHECK(mm["Model"]["a"].is_str() == false);
        CHECK(mm["Model"]["a"].type.value() == "A");
        CHECK(mm["Model"]["a1"].is_str() == false);
        CHECK(mm["Model"]["a1"].type.value() == "A1");
        CHECK(mm["Model"]["a2"].is_str() == false);
        CHECK(mm["Model"]["a2"].type.value() == "A2");
        //std::cout << mm << "\n";
    }
}

TEST_CASE("metamodel_simple_assignment_multiplicity1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            A1: x='a1' x='a2';             // multi
            A2: (x='a2' | x='a3') x='a1';  // multi
            A3: (x='a2' | x='a3');         // single
        )";

        textx::Metamodel mm{grammar1};

        CHECK(mm["A1"].type() == textx::RuleType::common);
        CHECK(mm["A2"].type() == textx::RuleType::common);
        CHECK(mm["A3"].type() == textx::RuleType::common);
        CHECK(mm["A1"]["x"].is_str() == true);
        CHECK(mm["A2"]["x"].is_str() == true);
        CHECK(mm["A3"]["x"].is_str() == true);
        CHECK(!mm["A1"]["x"].type.has_value());
        CHECK(!mm["A2"]["x"].type.has_value());
        CHECK(!mm["A3"]["x"].type.has_value());
        CHECK(mm["A1"]["x"].cardinality == textx::AttributeCardinality::list);
        CHECK(mm["A2"]["x"].cardinality == textx::AttributeCardinality::list);
        CHECK(mm["A3"]["x"].cardinality == textx::AttributeCardinality::scalar);
    }
}

TEST_CASE("metamodel_simple_obj_ref1", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'value' '=' (a=[A1]|a=[A2]) a1=[A1] a2*=[A2|MYID];
            A: A1|A2;
            A1: x='a1';
            A2: x='a2';
            MYID: /\d+\b/;
        )";

        textx::Metamodel mm{grammar1};
        CHECK_THROWS(mm.parsetree_from_str("value="));
        CHECK(mm.parsetree_from_str("value=t1 t1 2"));
        CHECK(mm.parsetree_from_str("value=t2 t1 2 3 4"));
        CHECK(mm.parsetree_from_str("value=t1 t1"));
        CHECK_THROWS(mm.parsetree_from_str("value=1 t1 2"));
        CHECK_THROWS(mm.parsetree_from_str("value=t1 t1 t2"));

        CHECK(mm["Model"]["a"].cardinality == textx::AttributeCardinality::scalar);
        CHECK(mm["Model"]["a1"].cardinality == textx::AttributeCardinality::scalar);
        CHECK(mm["Model"]["a2"].cardinality == textx::AttributeCardinality::list);
        CHECK(mm["A1"].type() == textx::RuleType::common);
        CHECK(mm["A2"].type() == textx::RuleType::common);
        CHECK(mm["A"].type() == textx::RuleType::abstract);
        CHECK(mm["Model"]["a"].is_str() == false);
        CHECK(mm["Model"]["a"].type.value() == "A");
        CHECK(mm["Model"]["a1"].is_str() == false);
        CHECK(mm["Model"]["a1"].type.value() == "A1");
        CHECK(mm["Model"]["a2"].is_str() == false);
        CHECK(mm["Model"]["a2"].type.value() == "A2");
    }
}

TEST_CASE("metamodel_boolean_assignment", "[textx/metamodel]")
{
    {
        auto grammar1 = R"(
            Model: 'value' a?='A' b?='B';
        )";
        auto mm = textx::metamodel_from_str(grammar1);
        CHECK( (*mm)["Model"]["a"].cardinality == textx::AttributeCardinality::scalar );
        CHECK( (*mm)["Model"]["a"].is_boolean() );
    }
    {
        auto grammar1 = R"(
            Model: 'value' a='B';
        )";
        auto mm = textx::metamodel_from_str(grammar1);
        CHECK( (*mm)["Model"]["a"].cardinality == textx::AttributeCardinality::scalar );
        CHECK( (*mm)["Model"]["a"].is_str() );
        CHECK( (*mm)["Model"]["a"].maybe_str() );

        CHECK( !(*mm)["Model"]["a"].is_boolean() );
        CHECK( !(*mm)["Model"]["a"].maybe_boolean() );
        CHECK( !(*mm)["Model"]["a"].maybe_obj() );
    }
    {
        auto grammar1 = R"(
            Model: 'value' a?='A' a?='B';
        )";
        CHECK_THROWS_WITH(textx::metamodel_from_str(grammar1), Catch::Matchers::Contains("no list of booleans"));
    }
    {
        auto grammar1 = R"(
            Model: 'value' (a?='A')*;
        )";
        CHECK_THROWS_WITH(textx::metamodel_from_str(grammar1), Catch::Matchers::Contains("no list of booleans"));
    }
}
TEST_CASE("metamodel_with_obj_attributes_testing_multitype_info1", "[textx/model]")
{
    {
        auto grammar1 = R"(
            Model: 'value' a='B' | a=C;
            C: x='C';
        )";
        auto mm = textx::metamodel_from_str(grammar1);
        CHECK(!(*mm)["Model"]["a"].maybe_boolean());
        CHECK((*mm)["Model"]["a"].maybe_str());
        CHECK((*mm)["Model"]["a"].maybe_obj());
        CHECK((*mm)["Model"]["a"].is_multi_type());

        CHECK(!(*mm)["Model"]["a"].is_boolean());
        CHECK(!(*mm)["Model"]["a"].is_str());
        CHECK(!(*mm)["Model"]["a"].is_obj());
    }

    {
        auto grammar1 = R"(
            Model: 'value' a='B' | a=C;
            C: 'C';
        )";
        auto mm = textx::metamodel_from_str(grammar1);
        CHECK((*mm)["Model"]["a"].maybe_str());
        CHECK((*mm)["Model"]["a"].is_str());

        CHECK(!(*mm)["Model"]["a"].maybe_boolean());
        CHECK(!(*mm)["Model"]["a"].is_multi_type());
        CHECK(!(*mm)["Model"]["a"].maybe_obj());
        CHECK(!(*mm)["Model"]["a"].is_boolean());
        CHECK(!(*mm)["Model"]["a"].is_obj());
    }

    {
        auto grammar1 = R"(
            Model: 'value' a='B';
        )";
        auto mm = textx::metamodel_from_str(grammar1);
        CHECK((*mm)["Model"]["a"].maybe_str());
        CHECK((*mm)["Model"]["a"].is_str());

        CHECK(!(*mm)["Model"]["a"].maybe_boolean());
        CHECK(!(*mm)["Model"]["a"].is_multi_type());
        CHECK(!(*mm)["Model"]["a"].maybe_obj());  // !
        CHECK(!(*mm)["Model"]["a"].is_boolean());
        CHECK(!(*mm)["Model"]["a"].is_obj());
    }

    {
        auto grammar1 = R"(
            Model: 'value' a?='A';
        )";
        auto mm = textx::metamodel_from_str(grammar1);
        CHECK((*mm)["Model"]["a"].maybe_boolean());
        CHECK((*mm)["Model"]["a"].is_boolean());

        CHECK(!(*mm)["Model"]["a"].is_multi_type());
        CHECK(!(*mm)["Model"]["a"].maybe_str());
        CHECK(!(*mm)["Model"]["a"].maybe_obj());  // !
        CHECK(!(*mm)["Model"]["a"].is_str());
        CHECK(!(*mm)["Model"]["a"].is_obj());
    }
}

TEST_CASE("metamodel_with_obj_attributes_testing_multitype_info2_advanced_abstract_rules", "[textx/model]")
{
    {
        auto grammar1 = R"(
            Model: 'value' a=C b=K c=N d=O e=P q=Q l=L;
            C: D|E;
            D: 'D';
            E: F;
            F: 'F';
            K: L|M;
            L: 'L' name=ID;
            M: 'M' name=ID;
            N: L|M|E;
            O: L|M|'test';
            P: '1'|'2'|'3';
            Q: '1'|'2'|'3' L;
            R: E L;
            S: E L | M;
            T: M;
        )";
        auto mm = textx::metamodel_from_str(grammar1);
        CHECK((*mm)["Model"].type() == textx::RuleType::common);
        CHECK((*mm)["C"].type() == textx::RuleType::match);
        CHECK((*mm)["D"].type() == textx::RuleType::match);
        CHECK((*mm)["E"].type() == textx::RuleType::match);
        CHECK((*mm)["F"].type() == textx::RuleType::match);
        CHECK((*mm)["K"].type() == textx::RuleType::abstract);
        CHECK((*mm)["L"].type() == textx::RuleType::common);
        CHECK((*mm)["M"].type() == textx::RuleType::common);
        CHECK((*mm)["N"].type() == textx::RuleType::abstract);
        CHECK((*mm)["O"].type() == textx::RuleType::abstract);
        CHECK((*mm)["P"].type() == textx::RuleType::match);
        CHECK((*mm)["Q"].type() == textx::RuleType::abstract);
        CHECK((*mm)["R"].type() == textx::RuleType::abstract); // to be discussed (how is this in original textx?)
        CHECK((*mm)["S"].type() == textx::RuleType::abstract);
        CHECK((*mm)["T"].type() == textx::RuleType::abstract);

        CHECK((*mm)["Model"]["a"].is_str());
        CHECK((*mm)["Model"]["b"].is_obj());

        CHECK((*mm)["Model"]["c"].is_multi_type());
        CHECK((*mm)["Model"]["c"].maybe_obj());
        CHECK((*mm)["Model"]["c"].maybe_str());
        CHECK(!(*mm)["Model"]["c"].maybe_boolean());

        CHECK((*mm)["Model"]["d"].is_multi_type());
        CHECK((*mm)["Model"]["d"].maybe_obj());
        CHECK((*mm)["Model"]["d"].maybe_str());
        CHECK(!(*mm)["Model"]["d"].maybe_boolean());

        CHECK((*mm)["Model"]["e"].is_str());

        CHECK((*mm)["Model"]["q"].is_multi_type());
        CHECK((*mm)["Model"]["q"].maybe_obj());
        CHECK((*mm)["Model"]["q"].maybe_str());
        CHECK(!(*mm)["Model"]["q"].maybe_boolean());

        CHECK((*mm)["Model"]["l"].is_obj());
    }
}

TEST_CASE("metamodel_with_cyclic_inh", "[textx/model]")
{
    auto grammar1 = R"(
        Model: 'value' a=C b=K c=N d=O e=P q=Q l=L;
        K: L|M;
        L: 'L' name=ID;
        M: L|K;
    )";
    CHECK_THROWS(textx::metamodel_from_str(grammar1));
}

TEST_CASE("metamodel_abstract_rules_with_sequences_of_rules_only_using_the_first_rule", "[textx/model]")
{
    // Example with "Base: Special1| (Special2 NotSpecial3)"
    // "If there are multiple common rules than the first will be used as a result and the rest only for parsing" from
    auto grammar1 = R"(
        Model: 'value' x=Base;
        Base: S1 | S2 NotS3 // only use first rule (S2) as
                            // possible instance for each choice-option
            | 'test1' ( ('test2' S4) | S5) S6; // Both S4 and S5 should be inheriting classes
                            // but not S6 as it comes second in sequence.
        S1: name=ID;
        S2: name=ID;
        NotS3: name=ID;
        S4: name=ID;
        S5: name=ID;
        S6: name=ID;
    )";
    auto mm = textx::metamodel_from_str(grammar1);
    CHECK( mm->is_instance("S1","Base") );
    CHECK( mm->is_instance("S2","Base") );
    CHECK( mm->is_instance("S4","Base") );
    CHECK( mm->is_instance("S5","Base") );
    CHECK( !mm->is_instance("NotS3","Base") );
    CHECK( !mm->is_instance("S6","Base") );
    CHECK( (*mm)["Base"].tx_inh_by().size()==4 );
}
