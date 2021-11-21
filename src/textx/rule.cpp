#include "textx/rule.h"
#include <unordered_map>
#include <cassert>
namespace {
    using RULE = textx::Rule;
    using GRAMMAR = textx::Grammar<RULE>;
    namespace ta = textx::arpeggio;

    textx::arpeggio::Pattern transform_match2pattern(GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match);

    ta::Pattern normal_expression_or_unordered_choice(GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match, bool use_choice) {
        auto &expr = match;
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
                }
                
                // match suppression
                assert(match.children[2].captured.has_value()); // match suppression '-'
                bool has_match_suppression = (match.children[2].captured.value()=="-");
                // TODO: use has_match_suppression

                // repeat modifiers
                std::string repeat_modifiers = match.children[1].captured.value(); 
                // TODO eval, use...

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
                    throw std::runtime_error(std::string("unexpected: op not covered: ")+op);
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
    };

    textx::arpeggio::Pattern transform_match2pattern(GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) {
        if (transform_match2pattern_map.count(match.name.value())==1) {
            try {
                return transform_match2pattern_map[match.name.value()](grammar, rule, match);
            }
            catch(std::exception& e) {
                std::ostringstream o;
                o << "error near " << match.start() << ".." << match.end() << ":" << e.what();
                throw std::runtime_error(o.str());
            }
        }
        else {
            throw std::runtime_error(std::string("unexpected: no entry in transform_match2pattern_map for ")+match.name.value());
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