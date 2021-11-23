#include "textx/rule.h"
#include <unordered_map>
#include "textx/assert.h"

namespace {
    using RULE = textx::Rule;
    using GRAMMAR = textx::Grammar<RULE>;
    namespace ta = textx::arpeggio;

    textx::arpeggio::Pattern transform_match2pattern(GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match);
    struct Eolterm{};
    using Repeat_modifiers = std::optional<std::variant<ta::Pattern, Eolterm>>;   
    Repeat_modifiers get_repeat_modifiers(GRAMMAR &grammar, RULE& rule, const ta::Match& m);

    ta::Pattern normal_expression_or_unordered_choice(GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match, bool use_choice) {
        auto &expr = match;
        if(expr.children[0].name.has_value() && expr.children[0].name.value() == "assignment") {
            return transform_match2pattern(grammar, rule, expr.children[0]);
        }
        else {
            assert(expr.children[0].children[1].type() == ta::MatchType::ordered_choice);
            ta::Pattern part_of_expression;
            if (use_choice) {
                auto choice = expr.children[0].children[1].children[0];
                assert(choice.name.value()=="bracketed_choice");
                assert(choice.children[1].type() == ta::MatchType::sequence);
                assert(choice.children[1].children.size()==2);
                assert(choice.children[1].children[1].children.size()==0); // only one entry + 0*zero_or_more
                auto &seq = choice.children[1].children[0]; // "(" .#1. ")"
                std::vector<ta::Pattern> patterns;
                for(auto &c : seq.children) {
                    patterns.push_back(transform_match2pattern( grammar, rule, c ));
                }
                part_of_expression = ta::unordered_group(patterns);
            }
            else {
                part_of_expression = transform_match2pattern(grammar, rule, expr.children[0].children[1].children[0]);
            }
            if (expr.children[0].children[0].children.size()>0) {
                assert(expr.children[0].children[0].children.size()==1);
                std::string syntactic_predicate = expr.children[0].children[0].children[0].captured.value(); // "!" or "&"
                if (syntactic_predicate=="!") {
                    return ta::negative_lookahead(part_of_expression);
                }
                else {
                    assert(syntactic_predicate=="&");
                    return ta::positive_lookahead(part_of_expression);
                }
            }
            else {
                return part_of_expression;
            }
        }
    }

    std::unordered_map<std::string, std::function<textx::arpeggio::Pattern(GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match)>> transform_match2pattern_map = {
        {
            "textx_rule_body",
            [](GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) -> ta::Pattern {
                return transform_match2pattern(grammar, rule, match.children[0]);
            }
        },
        {
            "choice",
            [](GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) -> ta::Pattern {
                auto seq1 = match.children[0];
                auto zom_seq2 = match.children[1];
                std::vector<textx::arpeggio::Pattern> c{ transform_match2pattern(grammar, rule, seq1) };
                for (auto &inner_seq_with_two_entries: zom_seq2.children) {
                    c.emplace_back( transform_match2pattern( grammar, rule, inner_seq_with_two_entries.children[1]) );
                }
                return ta::ordered_choice(c);
            }
        },
        {
            "sequence",
            [](GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) -> ta::Pattern {
                std::vector<textx::arpeggio::Pattern> c{};
                for (auto &inner_entry: match.children) {
                    c.emplace_back( transform_match2pattern( grammar, rule, inner_entry ));
                }
                return ta::sequence(c);
            }
        },
        {
            "repeatable_expr",
            [](GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) -> ta::Pattern {
                // operator *+#?
                // repeat modifier [',']
                assert(match.children[1].captured.has_value());
                std::string op = "";
                if (match.children[1].children.size()>0) {
                    assert(match.children[1].children[0].name.value() == "repeat_operator");
                    assert(match.children[1].children[0].children[0].name.value() == "repeat_operator_text");
                    op = match.children[1].children[0].children[0].captured.value(); // operator *+#?

                    // // repeat modifiers
                    // auto repeat_modifiers = match.children[1].children[0].children[1]; 
                    // if (repeat_modifiers.children.size()>0) {
                    //     assert(repeat_modifiers.children.size()<=1 && "repeat modifiers must containe ONE modifier");
                    //     auto repeat_modifiers_val = get_repeat_modifiers(grammar, rule, repeat_modifiers.children[0]);
                    //     // TODO eval, use... a[','] / a[eolterm]
                    // }
                }
                
                // match suppression
                assert(match.children[2].captured.has_value()); // match suppression '-'
                bool has_match_suppression = (match.children[2].captured.value()=="-");
                // TODO: use has_match_suppression

                // expression
                auto expression = normal_expression_or_unordered_choice( grammar, rule, match.children[0], op=="#");

                // create pattern
                if (op=="") {
                    return expression;
                }
                else if (op=="*") {
                    return ta::zero_or_more(expression);
                }
                else if (op=="+") {
                    return ta::one_or_more(expression);
                }
                else if (op=="#") {
                    return expression;
                }
                else {
                    ta::raise(match.start(), std::string("unexpected: op not covered: ")+op);
                }
            }
        },
        {
            "expression",
            [](GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) -> ta::Pattern {
                return transform_match2pattern( grammar, rule, match.children[0] );
            }
        },
        {
            "simple_match",
            [](GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) -> ta::Pattern {
                return transform_match2pattern( grammar, rule, match.children[0] );
            }
        },
        {
            "str_match",
            [](GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) -> ta::Pattern {
                std::string str = match.captured.value();
                assert(str.size()>=2);
                str = str.substr(1,str.size()-2);
                return ta::str_match(str);
            }
        },
        {
            "re_match",
            [](GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) -> ta::Pattern {
                std::string str = match.captured.value();
                assert(str.size()>=2);
                str = str.substr(1,str.size()-2);
                return ta::capture(ta::regex_match(str));
            }
        },
        {
            "bracketed_choice",
            [](GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) -> ta::Pattern {
                return transform_match2pattern( grammar, rule, match.children[1] );
            }
        },
        {
            "assignment",
            [](GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) -> ta::Pattern {
                assert(match.children.size()==3 && "assignment must have 3 children");
                assert(match.children[0].captured.has_value() && "assignment name");
                assert(match.children[1].captured.has_value() && "asisgnment op");
                auto attribute = match.children[0].captured.value();
                auto assignment_op = match.children[1].captured.value();
 
                auto &choice = match.children[2].children[0];
                auto assignment_rhs_content = transform_match2pattern( grammar, rule, choice.children[0] ); 
                //TODO
                //std::string assignment_rhs_repeat_modifiers = match.children[2].children[1].captured.value(); 

                // repeat modifiers
                auto repeat_modifiers = match.children[2].children[1]; 
                if (repeat_modifiers.children.size()>0) {
                    assert(repeat_modifiers.children.size()<=1 && "repeat modifiers must containe ONE modifier");
                    auto repeat_modifiers_val = get_repeat_modifiers(grammar, rule, repeat_modifiers.children[0]);
                    // TODO eval, use... a[','] / a[eolterm]
                }

                if (assignment_op=="=") {
                    // TODO handle assignment
                    assert(match.children[2].children[1].children.size()==0); //no mods
                    return assignment_rhs_content;
                }
                else if (assignment_op=="*=") {
                    // TODO handle assignment
                    // TODO repeat modifiers
                    return ta::zero_or_more(assignment_rhs_content);
                }
                else if (assignment_op=="+=") {
                    // TODO handle assignment
                    // TODO repeat modifiers
                    return ta::one_or_more(assignment_rhs_content);
                }
                else if (assignment_op=="?=") {
                    // TODO handle assignment
                    // TODO repeat modifiers
                    return ta::optional(assignment_rhs_content);
                }
                else {
                    ta::raise(match.start(),"unexpected assignment_op");
                }
                //return ta::str_match("todo");
            }
        },
    };

    Repeat_modifiers get_repeat_modifiers(GRAMMAR &grammar, RULE& rule, const ta::Match& m) {
        TEXTX_ASSERT(m.name.has_value(), "has_name expected");
        TEXTX_ASSERT_EQUAL(m.name.value(), "repeat_modifiers");
        auto second = m.children[1];
        if (second.captured.value()=="eolterm") {
            return Eolterm{};
        }
        else {
            return transform_match2pattern(grammar, rule, second.children[0]);
        }
    }

   textx::arpeggio::Pattern transform_match2pattern(GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) {
        if (transform_match2pattern_map.count(match.name.value())==1) {
            try {
                return transform_match2pattern_map[match.name.value()](grammar, rule, match);
            }
            catch(ta::Exception& e) {
                throw;
            }
            catch(std::exception& e) {
                std::ostringstream o;
                o << "error near " << match.start() << ".." << match.end() << ":" << e.what();
                ta::raise(match.start(), o.str());
            }
        }
        else {
            ta::raise(match.start(), "unexpected: no entry in transform_match2pattern_map for ", match.name.value());
        }
    }
}

namespace textx {

    Rule createRuleFromTextxPattern(textx::Grammar<textx::Rule>& grammar, std::string_view name, textx::arpeggio::Match rule_params, const textx::arpeggio::Match& rule_body, bool add_eof) {
        // << rule_body << "\n";
        Rule rule;
        rule.pattern = transform_match2pattern(grammar, rule, rule_body);
        if (add_eof) {
            rule.pattern = ta::sequence({rule.pattern, ta::end_of_file()});
        }
        return rule;
    }

}