#include "textx/rule.h"
#include <unordered_map>
#include "textx/assert.h"
#include "textx/metamodel.h"

namespace {

    using RULE = textx::parsetree::RuleInfo;
    using METAMODEL = textx::Metamodel;
    namespace ta = textx::arpeggio;

    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    struct ParseState {
        bool in_assignment=false;
    };

    ta::Pattern transform_match2pattern(ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match);

    struct Eolterm{};
    struct None{};
    using Repeat_modifiers = std::variant<ta::Pattern, Eolterm, None>;
    Repeat_modifiers get_repeat_modifiers(ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& m);

    /** get the repat modifiers info from a Match from ta::optional(ref("repeat_modifiers")) */
    Repeat_modifiers extract_repeat_modifiers(ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& repeat_modifiers_match) {
        Repeat_modifiers repeat_modifiers = None{};
        if (repeat_modifiers_match.children.size()>0) {
            TEXTX_ASSERT(repeat_modifiers_match.children[0].name.has_value());
            TEXTX_ASSERT_EQUAL(repeat_modifiers_match.children[0].name.value(), "rule://repeat_modifiers");
            TEXTX_ASSERT(repeat_modifiers_match.children.size()<=1, "repeat modifiers must containe ONE modifier");
            repeat_modifiers = get_repeat_modifiers(parsestate, mm, rule, repeat_modifiers_match.children[0]);
        }
        return repeat_modifiers;
    }

    /** preprocess an expression with a special case for unordered_groups */
    ta::Pattern normal_expression_or_unordered_choice(ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match, bool use_choice, const Repeat_modifiers &repeat_modifiers) {
        auto &expr = match;
        if(expr.children[0].name.has_value() && expr.children[0].name.value() == "rule://assignment") {
            return transform_match2pattern(parsestate, mm, rule, expr.children[0]);
        }
        else {
            TEXTX_ASSERT_EQUAL(expr.children[0].children[1].type(), ta::MatchType::ordered_choice);
            ta::Pattern part_of_expression;
            if (use_choice) {
                auto choice = expr.children[0].children[1].children[0];
                TEXTX_ASSERT_EQUAL(choice.name.value(),"rule://bracketed_choice");
                TEXTX_ASSERT_EQUAL(choice.children[1].type(), ta::MatchType::sequence);
                TEXTX_ASSERT_EQUAL(choice.children[1].children.size(),2);
                TEXTX_ASSERT_EQUAL(choice.children[1].children[1].children.size(),0); // only one entry + 0*zero_or_more
                auto &seq = choice.children[1].children[0]; // "(" .#1. ")"
                std::vector<ta::Pattern> patterns;
                for(auto &c : seq.children) {
                    patterns.push_back(transform_match2pattern( parsestate, mm, rule, c));
                }
                if (std::holds_alternative<ta::Pattern>(repeat_modifiers)) {
                    part_of_expression = ta::unordered_group(patterns, std::get<ta::Pattern>(repeat_modifiers));
                }
                else {
                    part_of_expression = ta::unordered_group(patterns);
                }
            }
            else {
                part_of_expression = transform_match2pattern(parsestate, mm, rule, expr.children[0].children[1].children[0]);
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

    /** normal node "visitors" */
    std::unordered_map<std::string, std::function<ta::Pattern(ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match)>> transform_match2pattern_map = {
        {
            "rule://textx_rule_body",
            [](ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) -> ta::Pattern {
                return transform_match2pattern(parsestate, mm, rule, match.children[0]);
            }
        },
        {
            "rule://choice",
            [](ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) -> ta::Pattern {
                auto seq1 = match.children[0];
                auto zom_seq2 = match.children[1];
                std::vector<ta::Pattern> c{ transform_match2pattern(parsestate, mm, rule, seq1) };
                for (auto &inner_seq_with_two_entries: zom_seq2.children) {
                    c.emplace_back( transform_match2pattern( parsestate, mm, rule, inner_seq_with_two_entries.children[1]) );
                }
                return ta::ordered_choice(c);
            }
        },
        {
            "rule://sequence",
            [](ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) -> ta::Pattern {
                std::vector<ta::Pattern> c{};
                for (auto &inner_entry: match.children) {
                    c.emplace_back( transform_match2pattern( parsestate, mm, rule, inner_entry));
                }
                return ta::sequence(c);
            }
        },
        {
            "rule://repeatable_expr",
            [](ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) -> ta::Pattern {
                // operator *+#?
                TEXTX_ASSERT(match.children[1].captured.has_value());
                std::string op = "";
                Repeat_modifiers repeat_modifiers = None{};
                if (match.children[1].children.size()>0) {
                    TEXTX_ASSERT_EQUAL(match.children[1].children[0].name.value(), "rule://repeat_operator");
                    TEXTX_ASSERT_EQUAL(match.children[1].children[0].children[0].name.value(), "rule://repeat_operator_text");
                    op = match.children[1].children[0].children[0].captured.value(); // operator *+#?

                    // repeat modifiers
                    auto repeat_modifiers_match = match.children[1].children[0].children[1];
                    repeat_modifiers = extract_repeat_modifiers(parsestate, mm, rule, repeat_modifiers_match);
                    // TODO eval, use... a[','] / a[eolterm]
                }
                
                // match suppression
                TEXTX_ASSERT(match.children[2].captured.has_value()); // match suppression '-'
                bool has_match_suppression = (match.children[2].captured.value()=="-");
                // TODO: use has_match_suppression

                // expression
                auto expression = normal_expression_or_unordered_choice( parsestate, mm, rule, match.children[0], op=="#", repeat_modifiers);

                // create pattern
                if (op=="") {
                    return expression;
                }
                else if (op=="*") {
                   return std::visit(overloaded{
                        [&](ta::Pattern&p) -> ta::Pattern { return ta::optional(ta::sequence({expression, ta::zero_or_more(ta::sequence({p, expression}))})); },
                        [&](Eolterm&) -> ta::Pattern { ta::raise(match.start(), "TODO eolterm"); },
                        [&](None&) -> ta::Pattern { return ta::zero_or_more(expression); }
                    }, repeat_modifiers);
                }
                else if (op=="+") {
                   return std::visit(overloaded{
                        [&](ta::Pattern&p) -> ta::Pattern { return ta::sequence({expression, ta::zero_or_more(ta::sequence({p, expression}))}); },
                        [&](Eolterm&) -> ta::Pattern { ta::raise(match.start(), "TODO eolterm"); },
                        [&](None&) -> ta::Pattern { return ta::one_or_more(expression); }
                    }, repeat_modifiers);
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
            "rule://expression",
            [](ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) -> ta::Pattern {
                return transform_match2pattern( parsestate, mm, rule, match.children[0]);
            }
        },
        {
            "rule://simple_match",
            [](ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) -> ta::Pattern {
                return transform_match2pattern( parsestate, mm, rule, match.children[0]);
            }
        },
        {
            "rule://str_match",
            [](ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) -> ta::Pattern {
                std::string str = match.captured.value();
                TEXTX_ASSERT(str.size()>=2);
                str = str.substr(1,str.size()-2);
                return ta::str_match(str);
            }
        },
        {
            "rule://re_match",
            [](ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) -> ta::Pattern {
                std::string str = match.captured.value();
                TEXTX_ASSERT(str.size()>=2);
                str = str.substr(1,str.size()-2);
                return ta::capture(ta::regex_match(str));
            }
        },
        {
            "rule://bracketed_choice",
            [](ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) -> ta::Pattern {
                return transform_match2pattern( parsestate, mm, rule, match.children[1]);
            }
        },
        {
            "rule://assignment",
            [](ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) -> ta::Pattern {
                parsestate.in_assignment = true;
                TEXTX_ASSERT_EQUAL(match.children.size(),3 , "assignment must have 3 children");
                TEXTX_ASSERT(match.children[0].captured.has_value(), "assignment name");
                TEXTX_ASSERT(match.children[1].captured.has_value(), "asisgnment op");
                auto attribute_name = match.children[0].captured.value();
                auto assignment_op = match.children[1].captured.value();

                auto &choice = match.children[2].children[0];
                ta::Pattern assignment_rhs_content = transform_match2pattern( parsestate, mm, rule, choice.children[0]); 

                // repeat modifiers
                auto repeat_modifiers_match = match.children[2].children[1]; 
                Repeat_modifiers repeat_modifiers = extract_repeat_modifiers(parsestate, mm, rule, repeat_modifiers_match);
                // TODO eval, use... a[','] / a[eolterm]

                // TODO extract "type" / terminal

                // register attribute
                if (choice.children[0].name.value()=="rule://reference" && choice.children[0].children[0].name.value()=="rule://obj_ref") {
                    rule.add_attribute(attribute_name, choice.children[0].children[0].children[1].captured.value()); // add type here
                }
                else if (choice.children[0].name.value()=="rule://reference" && choice.children[0].children[0].name.value()=="rule://rule_ref") {
                    rule.add_attribute(attribute_name, choice.children[0].children[0].captured.value());
                }
                else {
                    rule.add_attribute(attribute_name); // add type here
                }

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
        {
            "rule://rule_ref",
            [](ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) -> ta::Pattern {
                auto ref_rule_name = match.captured.value();
                if (!parsestate.in_assignment) {
                    rule.add_tx_inh_by(ref_rule_name);
                }
                return mm.ref(ref_rule_name);
            }
        },
        {
            "rule://obj_ref",
            [](ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) -> ta::Pattern {
                auto &optional_format = match.children[2];
                std::string ref_rule_name = "ID";
                if (optional_format.children.size()>0) {
                    ref_rule_name = optional_format.children[0].children[1].captured.value();
                }
                return mm.ref(ref_rule_name);
            }
        },
        {
            "rule://reference",
            [](ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) -> ta::Pattern {
                return transform_match2pattern(parsestate, mm, rule, match.children[0]);
            }
        },
    };

    Repeat_modifiers get_repeat_modifiers(ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& m) {
        TEXTX_ASSERT(m.name.has_value(), "has_name expected");
        TEXTX_ASSERT_EQUAL(m.name.value(), "rule://repeat_modifiers");
        auto second = m.children[1];
        if (second.captured.value()=="eolterm") {
            return Eolterm{};
        }
        else {
            return transform_match2pattern(parsestate, mm, rule, second.children[0].children[0]);
        }
    }

   ta::Pattern transform_match2pattern(ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) {
        if (transform_match2pattern_map.count(match.name.value())==1) {
            try {
                return transform_match2pattern_map[match.name.value()](parsestate, mm, rule, match);
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

    Rule createRuleFromTextxPattern(textx::Metamodel& mm, std::string_view name, ta::Match rule_params, const ta::Match& rule_body, textx::parsetree::RuleInfo& rule_info, bool add_eof) {
        // << rule_body << "\n";
        Rule rule;
        rule.name = name;
        rule.pattern = transform_match2pattern(ParseState{}, mm, rule_info, rule_body);
        if (add_eof) {
            rule.pattern = ta::sequence({rule.pattern, ta::end_of_file()});
        }
        rule.m_type = RuleType::illegal;
        return rule;
    }
}
