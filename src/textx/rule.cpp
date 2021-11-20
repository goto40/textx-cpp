#include "textx/rule.h"
#include <unordered_map>
#include <cassert>
namespace {
    using RULE = textx::Rule;
    using GRAMMAR = textx::Grammar<RULE>;
    namespace ta = textx::arpeggio;

    textx::arpeggio::Pattern transform_match2pattern(GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match);

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
                // expression
                auto expression = transform_match2pattern( grammar, rule, match.children[0] );
                // operator *+#?
                assert(match.children[1].captured.has_value());
                std::string op = match.children[1].captured.value();
                // modifier [',']
                // match suppression '-'
                // create pattern
                return ta::str_match("TODO");
            }
        },
        {
            "expression",
            [](GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) -> ta::Pattern {
                if (match.children[0].type()==ta::MatchType::sequence) {
                    auto &seq = match.children[0];
                    assert(seq.children[1].type() == ta::MatchType::ordered_choice);
                    auto part_of_expression = transform_match2pattern(grammar, rule, seq.children[1].children[0]);
                    if (seq.children[0].children.size()>0) {
                        std::string syntactic_predicate = seq.children[0].children[0].captured.value(); // "!" or "&"
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
                else { // assignment
                    return transform_match2pattern( grammar, rule, match.children[0] );
                }
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
    };

    textx::arpeggio::Pattern transform_match2pattern(GRAMMAR &grammar, RULE& rule, const textx::arpeggio::Match& match) {
        if (transform_match2pattern_map.count(match.name.value())==1) {
            return transform_match2pattern_map[match.name.value()](grammar, rule, match);
        }
        else {
            throw std::runtime_error(std::string("unexpected: no entry in transform_match2pattern_map for ")+match.name.value());
        }
    }
}

namespace textx {

    Rule createRuleFromTextxPattern(textx::Grammar<textx::Rule>& grammar, std::string_view name, textx::arpeggio::Match rule_params, const textx::arpeggio::Match& rule_body) {
        std::cout << rule_body << "\n";
        Rule rule;
        rule.pattern = transform_match2pattern(grammar, rule, rule_body);
        return rule;
    }

}