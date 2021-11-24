#include "textx/rule.h"
#include <unordered_map>
#include "textx/assert.h"

namespace {
    using RULE = textx::Rule;
    using GRAMMAR = textx::Grammar<RULE>;
    namespace ta = textx::arpeggio;

    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    textx::arpeggio::Pattern transform_match2pattern(GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match);
    struct Eolterm{};
    struct None{};
    using Repeat_modifiers = std::variant<ta::Pattern, Eolterm, None>;
    Repeat_modifiers get_repeat_modifiers(GRAMMAR &grammar, RULE& rule, const ta::Match& m);

    ta::Pattern normal_expression_or_unordered_choice(GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match, bool use_choice) {
        auto &expr = match;
        if(expr.children[0].name.has_value() && expr.children[0].name.value() == "assignment") {
            return transform_match2pattern(grammar, rule, expr.children[0]);
        }
        else {
            TEXTX_ASSERT_EQUAL(expr.children[0].children[1].type(), ta::MatchType::ordered_choice);
            ta::Pattern part_of_expression;
            if (use_choice) {
                auto choice = expr.children[0].children[1].children[0];
                TEXTX_ASSERT_EQUAL(choice.name.value(),"bracketed_choice");
                TEXTX_ASSERT_EQUAL(choice.children[1].type(), ta::MatchType::sequence);
                TEXTX_ASSERT_EQUAL(choice.children[1].children.size(),2);
                TEXTX_ASSERT_EQUAL(choice.children[1].children[1].children.size(),0); // only one entry + 0*zero_or_more
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
                TEXTX_ASSERT_EQUAL(expr.children[0].children[0].children.size(), 1);
                std::string syntactic_predicate = expr.children[0].children[0].children[0].captured.value(); // "!" or "&"
                if (syntactic_predicate=="!") {
                    return ta::negative_lookahead(part_of_expression);
                }
                else {
                    TEXTX_ASSERT_EQUAL(syntactic_predicate,"&");
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
                TEXTX_ASSERT(match.children[1].captured.has_value());
                std::string op = "";
                if (match.children[1].children.size()>0) {
                    TEXTX_ASSERT_EQUAL(match.children[1].children[0].name.value(), "repeat_operator");
                    TEXTX_ASSERT_EQUAL(match.children[1].children[0].children[0].name.value(), "repeat_operator_text");
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
                TEXTX_ASSERT(match.children[2].captured.has_value()); // match suppression '-'
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
                TEXTX_ASSERT(str.size()>=2);
                str = str.substr(1,str.size()-2);
                return ta::str_match(str);
            }
        },
        {
            "re_match",
            [](GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) -> ta::Pattern {
                std::string str = match.captured.value();
                TEXTX_ASSERT(str.size()>=2);
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
                TEXTX_ASSERT_EQUAL(match.children.size(),3 , "assignment must have 3 children");
                TEXTX_ASSERT(match.children[0].captured.has_value(), "assignment name");
                TEXTX_ASSERT(match.children[1].captured.has_value(), "asisgnment op");
                auto attribute = match.children[0].captured.value();
                auto assignment_op = match.children[1].captured.value();
 
                auto &choice = match.children[2].children[0];
                auto assignment_rhs_content = transform_match2pattern( grammar, rule, choice.children[0] ); 
                //TODO
                //std::string assignment_rhs_repeat_modifiers = match.children[2].children[1].captured.value(); 

                // repeat modifiers
                Repeat_modifiers repeat_modifiers = None{};
                auto repeat_modifiers_match = match.children[2].children[1]; 
                if (repeat_modifiers_match.children.size()>0) {
                    TEXTX_ASSERT(repeat_modifiers_match.children.size()<=1, "repeat modifiers must containe ONE modifier");
                    repeat_modifiers = get_repeat_modifiers(grammar, rule, repeat_modifiers_match.children[0]);
                }
                // TODO eval, use... a[','] / a[eolterm]

                if (assignment_op=="=") {
                    // TODO handle assignment
                    TEXTX_ASSERT(std::holds_alternative<None>(repeat_modifiers),"no repeat modifiers allowed for an assignment");
                    return assignment_rhs_content;
                }
                else if (assignment_op=="*=") {
                    // TODO handle assignment
                    return std::visit(overloaded{
                        [&](ta::Pattern&p) -> ta::Pattern { return ta::optional(ta::sequence({assignment_rhs_content, ta::zero_or_more(ta::sequence({p, assignment_rhs_content}))})); },
                        [&](Eolterm&) -> ta::Pattern { ta::raise(repeat_modifiers_match.start(), "TODO eolterm"); },
                        [&](None&) -> ta::Pattern { return ta::zero_or_more(assignment_rhs_content); }
                    }, repeat_modifiers);
                }
                else if (assignment_op=="+=") {
                    // TODO handle assignment
                    return std::visit(overloaded{
                        [&](ta::Pattern&p) -> ta::Pattern { return ta::sequence({assignment_rhs_content, ta::zero_or_more(ta::sequence({p, assignment_rhs_content}))}); },
                        [&](Eolterm&) -> ta::Pattern { ta::raise(repeat_modifiers_match.start(), "TODO eolterm"); },
                        [&](None&) -> ta::Pattern { return ta::one_or_more(assignment_rhs_content); }
                    }, repeat_modifiers);
                }
                else if (assignment_op=="?=") {
                    // TODO handle assignment
                    TEXTX_ASSERT(std::holds_alternative<None>(repeat_modifiers),"no repeat modifiers allowed for a boolean assignment");
                    return ta::optional(assignment_rhs_content);
                }
                else {
                    ta::raise(match.start(),"unexpected assignment_op");
                }
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
            return transform_match2pattern(grammar, rule, second.children[0].children[0]);
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