#include "textx/rule.h"
#include "textx/assert.h"
#include "textx/metamodel.h"
#include "textx/rrel.h"
#include "textx/tools.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace {

    using RULE = textx::Rule;
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
        TEXTX_ASSERT(expr.name_starts_with("rule://expression"));
        if(expr.children[0].name.has_value() && expr.children[0].name.value() == "rule://assignment") {
            return transform_match2pattern(parsestate, mm, rule, expr.children[0]);
        }
        else {
            TEXTX_ASSERT_EQUAL(expr.children[0].children[1].type(), ta::MatchType::ordered_choice);
            ta::Pattern part_of_expression;
            if (use_choice) {
                auto choice = expr.children[0].children[1].children[0];
                TEXTX_ASSERT_EQUAL(choice.name.value(),"rule://bracketed_choice"); // must be "(" ... ")" for "#"
                TEXTX_ASSERT_EQUAL(choice.children[1].type(), ta::MatchType::sequence);
                TEXTX_ASSERT_EQUAL(choice.children[1].children.size(),2);

                std::vector<ta::Pattern> patterns={};
                if(choice.children[1].children[1].children.size()>0) {
                    // we have a choice here (e.g., "a|b|c")
                    auto &seq = choice.children[1].children[0]; // "(" .#1. ")"
                    patterns.push_back(transform_match2pattern(parsestate, mm, rule, seq));
                    for(auto &c : choice.children[1].children[1].children) {
                        patterns.push_back(transform_match2pattern( parsestate, mm, rule, c.children[1])); // see lang.cpp, use sequence after '|'
                    }
                }
                else {
                    auto &seq = choice.children[1].children[0]; // "(" .#1. ")"
                    for(auto &c : seq.children) {
                        patterns.push_back(transform_match2pattern( parsestate, mm, rule, c));
                    }
                }
                if (std::holds_alternative<ta::Pattern>(repeat_modifiers)) {
                    part_of_expression = ta::unordered_group(patterns, std::get<ta::Pattern>(repeat_modifiers));
                }
                else {
                    part_of_expression = ta::unordered_group(patterns, std::nullopt);
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

    textx::AttributeCardinality get_multiplicity(const textx::arpeggio::Match &match) {
        if (match.name.has_value()) {
            if (match.name.value() == "rule://assignment") {
                auto assignment_op = match.children[1].captured.value();
                if (assignment_op=="*=" || assignment_op=="+=") {
                    return textx::AttributeCardinality::list;
                }
                else if (assignment_op=="?=") {
                    return textx::AttributeCardinality::scalar;
                }
            }
            if (match.name.value() == "rule://repeatable_expr") {
                if (match.children[1].children.size()>0) {
                    std::string op = match.children[1].children[0].children[0].captured.value(); // operator *+#?
                    if (op=="*" || op=="+") {
                        return textx::AttributeCardinality::list;
                    }
                }
            }
        }
        return textx::AttributeCardinality::scalar;
    }

    bool is_assignment_to_attribute(const textx::arpeggio::Match &match, std::string name) {
        if (match.name.has_value()) {
            if (match.name.value() == "rule://assignment") {
                auto attribute_name = match.children[0].captured.value();
                //std::cout << "CHECK " << attribute_name << "==" << name << "\n"; 
                return attribute_name == name;
            }
        }
        return false;
    }

    bool is_rule(const textx::arpeggio::Match &match, std::string name) {
        if (match.name.has_value()) {
            return (match.name.value() == name);
        }
        return false;
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
                        [&](Eolterm&) -> ta::Pattern { return ta::eolterm(ta::zero_or_more(expression)); },
                        [&](None&) -> ta::Pattern { return ta::zero_or_more(expression); }
                    }, repeat_modifiers);
                }
                else if (op=="+") {
                    return std::visit(overloaded{
                        [&](ta::Pattern&p) -> ta::Pattern { return ta::sequence({expression, ta::zero_or_more(ta::sequence({p, expression}))}); },
                        [&](Eolterm&) -> ta::Pattern { return ta::eolterm(ta::one_or_more(expression)); },
                        [&](None&) -> ta::Pattern { return ta::one_or_more(expression); }
                    }, repeat_modifiers);
                }
                else if (op=="?") {
                    TEXTX_ASSERT(std::holds_alternative<None>(repeat_modifiers),"no repeat modifiers allowed for an optional assignment");
                    return ta::optional(expression);
                }
                else if (op=="#") {
                    return expression; // '#' handled in normal_expression_or_unordered_choice
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
                TEXTX_ASSERT(match.children[1].captured.has_value(), "assignment op");
                auto attribute_name = match.children[0].captured.value();
                auto assignment_op = match.children[1].captured.value();

                auto &choice = match.children[2].children[0];
                std::ostringstream assignment_info;
                if (assignment_op=="?=") {
                    assignment_info << "boolean_assignment://" << attribute_name;
                }
                else {
                    assignment_info << "assignment://" << attribute_name;
                }
                ta::Pattern assignment_rhs_content = ta::named(assignment_info.str(), ta::sequence({transform_match2pattern( parsestate, mm, rule, choice.children[0])})); 

                // repeat modifiers
                auto repeat_modifiers_match = match.children[2].children[1]; 
                Repeat_modifiers repeat_modifiers = extract_repeat_modifiers(parsestate, mm, rule, repeat_modifiers_match);
                // TODO eval, use... a[','] / a[eolterm]

                // TODO extract "type" / terminal

                // register attribute
                if (choice.children[0].name.value()=="rule://reference" && choice.children[0].children[0].name.value()=="rule://obj_ref") {
                    if (assignment_op=="?=") {
                        rule.add_attribute_with_boolean_type(attribute_name);
                    }
                    else {
                        rule.add_attribute_with_rule_type(attribute_name, choice.children[0].children[0].children[1].captured.value()); // add type here
                    }
                }
                else if (choice.children[0].name.value()=="rule://reference" && choice.children[0].children[0].name.value()=="rule://rule_ref") {
                    if (assignment_op=="?=") {
                        rule.add_attribute_with_boolean_type(attribute_name);
                    }
                    else {
                        rule.add_attribute_with_rule_type(attribute_name, choice.children[0].children[0].captured.value());
                    }
                }
                else {
                    if (assignment_op=="?=") {
                        rule.add_attribute_with_boolean_type(attribute_name);
                    }
                    else {
                        rule.add_attribute_with_str_type(attribute_name);
                    }
                }

                {
                    auto rrel_match = match.search("rule://rrel_expression");
                    if (rrel_match!=nullptr) {
                        std::string split_string = ".";
                        auto obj_ref = match.search("rule://obj_ref");
                        if (obj_ref!=nullptr) {
                            auto obj_ref_rule = obj_ref->search("rule://obj_ref_rule");
                            if (obj_ref_rule!=nullptr) {
                                auto &mm_rule = mm[obj_ref_rule->captured.value()];
                                if (mm_rule.tx_params().count("split")>0) {
                                    split_string = mm_rule.tx_params("split");
                                    TEXTX_ASSERT(split_string.size()>0);
                                }
                            }
                        }
                        mm.set_resolver(rule.tx_name()+"."+attribute_name, std::make_unique<textx::rrel::RRELScopeProvider>(*rrel_match,split_string));
                        //std::cout << "found: " << rule.name << "." << attribute_name << "\n";
                    }
                }

                if (assignment_op=="=") {
                    TEXTX_ASSERT(std::holds_alternative<None>(repeat_modifiers),"no repeat modifiers allowed for an assignment");
                    return assignment_rhs_content;
                }
                else if (assignment_op=="*=") {
                    return std::visit(overloaded{
                        [&](ta::Pattern&p) -> ta::Pattern { return ta::optional(ta::sequence({assignment_rhs_content, ta::zero_or_more(ta::sequence({p, assignment_rhs_content}))})); },
                        [&](Eolterm&) -> ta::Pattern { return ta::eolterm(ta::zero_or_more(assignment_rhs_content)); },
                        [&](None&) -> ta::Pattern { return ta::zero_or_more(assignment_rhs_content); }
                    }, repeat_modifiers);
                }
                else if (assignment_op=="+=") {
                    return std::visit(overloaded{
                        [&](ta::Pattern&p) -> ta::Pattern { return ta::sequence({assignment_rhs_content, ta::zero_or_more(ta::sequence({p, assignment_rhs_content}))}); },
                        [&](Eolterm&) -> ta::Pattern { return ta::eolterm(ta::one_or_more(assignment_rhs_content)); },
                        [&](None&) -> ta::Pattern { return ta::one_or_more(assignment_rhs_content); }
                    }, repeat_modifiers);
                }
                else if (assignment_op=="?=") {
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
                    //std::cout << "rule " << rule.tx_name() << " add_tx_inh_by+= " << ref_rule_name << "\n";
                }
                return mm.ref(ref_rule_name);
            }
        },
        {
            "rule://obj_ref",
            [](ParseState parsestate, METAMODEL &mm, RULE& rule, const ta::Match& match) -> ta::Pattern {
                auto type_name = match.children[1].captured.value();
                auto &optional_format = match.children[2];
                std::string ref_rule_name = "ID";
                if (optional_format.children.size()>0) {
                    ref_rule_name = optional_format.children[0].children[1].captured.value();
                }
                return named(std::string("obj_ref://")+type_name,ta::sequence({mm.ref(ref_rule_name)}));
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

    Rule::Rule(const textx::Metamodel& mm, std::string_view name, ta::Match rule_params, const ta::Match& rule_body) {
        // << rule_body << "\n";
        auto& rule = *this;
        rule.name = name;
        rule.m_type = RuleType::illegal;
        if (rule_params.children.size()>0) {
            auto &all_params = rule_params.children[0];
            auto add_param=[&](const ta::Match& p) {
                TEXTX_ASSERT(p.name_is("rule://rule_param"));
                std::string name = p.children[0].captured.value();
                std::string val = "";
                if (p.children[1].children.size()>0) {
                    val = p.children[1].children[0].children[1].captured.value();
                    TEXTX_ASSERT(val.size()>=2);
                    TEXTX_ASSERT(val[0] == val[val.size()-1]);
                    TEXTX_ASSERT(val[0] == '\'' || val[0] == '"');
                    val = val.substr(1,val.size()-2);
                    TEXTX_ASSERT(name=="split", "unexpected param with value ", name);
                }
                else {
                    TEXTX_ASSERT(name=="skipws" || name=="noskipws", "unexpected param ", name);
                }
                rule.m_tx_params[name]=val;
            };
            add_param(all_params.children[1]);
            for (const auto& c: all_params.children[2].children) {
                add_param(c.children[1]);
            }
        }
    }

    void Rule::post_process_created_rule(textx::Metamodel& mm, std::string_view name, ta::Match rule_params, const ta::Match& rule_body) {
        auto& rule = *this;
        rule.pattern = transform_match2pattern(ParseState{}, mm, rule, rule_body);
        rule.intern_arpeggio_rule_body = &rule_body;
        std::string rname = std::string("rule://")+std::string(name);
        for(auto& [k,v]: attribute_info) {
            v.cardinality = get_attribute_cardinality(rule_body, k);
        }
        rule.pattern = textx::arpeggio::rule(textx::arpeggio::named(rname,rule.pattern));
        if (rule.tx_params().count("noskipws")>0) {
            TEXTX_ASSERT(rule.tx_params().count("skipws")==0);
            rule.pattern = textx::arpeggio::noskipws(rule.pattern);
        }
        else if (rule.tx_params().count("skipws")>0) {
            rule.pattern = textx::arpeggio::skipws(rule.pattern);
        }
    }

    textx::AttributeCardinality Rule::get_attribute_cardinality(const textx::arpeggio::Match& match, std::string name) {
        TEXTX_ASSERT(attribute_info.count(name)>0);
        bool is_boolean_involved = false;
        std::function<size_t(const textx::arpeggio::Match&,size_t)> traverse;
        traverse = [&is_boolean_involved,&name,&traverse](const textx::arpeggio::Match& m,size_t c) -> size_t {
            if (get_multiplicity(m)==AttributeCardinality::list) {
                c = 2; // more than 1 --> list
            }
            if (is_assignment_to_attribute(m,name)) {
                c = std::max<decltype(c)>(c,1);
                TEXTX_ASSERT(m.children[1].captured.has_value(), "plausibility");
                if (m.children[1].captured.value()=="?=") {
                    is_boolean_involved = true;
                }
                return c;
            }
            else {
                size_t ret = 0; // no assignment
                if (is_rule(m,"rule://choice")) {
                    // add_rule("choice", 
                    //  0) ta::sequence({ref("sequence"), 
                    //  1) ta::zero_or_more(
                    //        ta::sequence({ta::str_match("|"),
                    //                      ref("sequence")}))}));
                    ret = traverse(m.children[0],c);
                    auto &zero_or_more = m.children[1];
                    for (auto &zero_or_more_child: zero_or_more.children) {
                        ret = std::max(ret, traverse(zero_or_more_child,c));
                    }
                }
                else {
                    for (auto &child: m.children) {
                        auto inner_ret = traverse(child,c);
                        ret += inner_ret;
                    }
                }
                return ret;
            }
        };
        size_t ret = traverse(match, 0);
        //std::cout /*<< match*/ << "\n=="<< ret << " for " << name << "\n\n";
        if (ret>1) {
            TEXTX_ASSERT(!is_boolean_involved, "only one boolean assignment is allowed (no list of booleans) for ", name, " in ", match.start());
            return AttributeCardinality::list;
        }
        else {
            return AttributeCardinality::scalar;
        }
    }

    void Rule::determine_rule_type_and_adjust_inh_by(const textx::Metamodel& mm) {
        // (a) determine inh_by-list (use only first common rule ref)
        // (b) determine hint for "RuleType::abstract" in "determine_rule_type_and_adjust_inh_by"
        // (d) add hint that rule can produce a string (e.g. Rule: Common|Match or Rule: Common|'str')

        // 1) nothing to for resolved cases or common rules
        if (attribute_info.size()>0) {
            m_type = RuleType::common;
            return;
        }
        if (type()!=RuleType::illegal) return;
        TEXTX_ASSERT(m_tx_inh_by.size()==0, "initially no classes should have been indentified (plausibility check)");
        TEXTX_ASSERT(intern_arpeggio_rule_body != nullptr, "plausibiliy");

        // 2) find inh-by list
        bool found_illegal=false;
        std::function<bool(const textx::arpeggio::Match&)> add_inh_by_rule;
        add_inh_by_rule = [&,this](const textx::arpeggio::Match& node) -> bool {
            bool found_non_match_here=false;
            if(node.name_is("rule://choice")) {
                // 2a) body -> (choice)
                // add_rule("choice", 
                //  0) ta::sequence({ref("sequence"), 
                //  1) ta::zero_or_more(
                //        ta::sequence({ta::str_match("|"),
                //                      ref("sequence")}))}));
                found_non_match_here = add_inh_by_rule(node.children[0]);
                if (!found_non_match_here) { m_maybe_str=true; }
                auto &zero_or_more = node.children[1];
                for (auto &zero_or_more_child: zero_or_more.children) {
                    bool f = add_inh_by_rule(zero_or_more_child);
                    if (!f) { m_maybe_str=true; }
                    found_non_match_here = f || found_non_match_here;
                }
            }
            else {
                if (node.name_is("rule://rule_ref")) {
                    std::string rule_name = node.captured.value();
                    if (mm[rule_name].m_type==RuleType::illegal) {
                        found_illegal=true;
                    }
                    if (!found_illegal) {
                        if (mm[rule_name].m_type!=RuleType::match) {
                            m_tx_inh_by.insert(rule_name);
                            found_non_match_here = true;
                        }
                    }
                }
                else {
                    for (auto &c: node.children) {
                        found_non_match_here = add_inh_by_rule(c) || found_non_match_here;
                        if (found_non_match_here) break;
                    }
                }
            }
            return found_non_match_here;
        };
        bool found_non_match=add_inh_by_rule(*intern_arpeggio_rule_body);

        if (found_illegal) {
            m_tx_inh_by.clear(); // retry later...
            m_maybe_str = false;  // retry later...
        }
        else {
            if (found_non_match) {
                m_type = RuleType::abstract;
            }
            else {
                m_type = RuleType::match;
            }
        }

        // add all inh-by-classes also referenced by all referenced inh-by-classes
        if (type()==RuleType::abstract) {
            auto me = mm.get_fqn_for_rule(name);
            auto copy = tx_inh_by();
            for(auto &c: copy) {
                auto &other_rule = mm[c];
                for(auto &t: other_rule.tx_inh_by()) {
                    if (tx_inh_by().count(t)==0) {
                        m_tx_inh_by.insert(t);
                    }
                }
            }
        }
    }

    void Rule::adjust_attr_types(const textx::Metamodel& mm) {
        for (auto &[name,info]: attribute_info) {
            info.adjust_type(mm);
        }
        // final check for boolean attr. to be "alone"
        for (auto &[name,info]: attribute_info) {
            if (info.maybe_boolean()) {
                TEXTX_ASSERT(info.is_boolean(), "boolean assignments must be alone and not mixed with other assignments for attr=",name)
            }
        }
    }

    void AttributeInfo::adjust_type(const textx::Metamodel& mm) {
        // remove all match types + adjust maybe_str
        auto rem_it = std::remove_if(
            types.begin(),
            types.end(),
            [&mm](const std::string &rule_name){ return mm[rule_name].type()==RuleType::match; }
        );
        if (rem_it != types.end()) {
            m_maybe_str = true; // found possible match rule
            types.erase(rem_it, types.end());
        }

        // determine base + adjust maybe_obj
        if (types.size()>0) {
            m_maybe_obj = true;
            for (std::string rule_name: types) {
                if (mm[rule_name].maybe_str()) {
                    m_maybe_str=true;
                }
            }
            std::unordered_set<std::string> type_set = {};
            for (auto& base: mm.tx_all_types()) {
                if (std::all_of(
                    types.begin(),
                    types.end(),
                    [&](auto &t){ return mm.is_base_of(base,t);}
                )) {
                    type_set.insert(base);
                }
            }
            if (type_set.size()==0) {
                type = "OBJECT";
            }
            else {
                // select one not the base of another; then smallest
                size_t n{type_set.size()}, n_old{};
                do {
                    if (type_set.size()==1) break;
                    n_old = n;
                    auto t0 = *type_set.begin();    
                    if (std::any_of(
                        types.begin(),
                        types.end(),
                        [&](auto &t){ return mm.is_base_of(t0,t) && t0!=t;}
                    )) {
                        type_set.erase(t0);
                    }
                    n = type_set.size();
                } while(n!=n_old);

                TEXTX_ASSERT(type_set.size()>0); // ==1??
                type = *type_set.begin();
            }
        }
    }

}
