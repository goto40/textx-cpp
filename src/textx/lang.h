#pragma once
#include "textx/arpeggio.h"

namespace textx
{
    /** adapted grammar from lang.py */
    namespace lang
    {
        inline textx::arpeggio::Pattern import_or_reference_stm();
        inline textx::arpeggio::Pattern import_stm();
        inline textx::arpeggio::Pattern reference_stm();
        inline textx::arpeggio::Pattern textx_rule();
        inline textx::arpeggio::Pattern grammar_to_import();
        inline textx::arpeggio::Pattern language_name();
        inline textx::arpeggio::Pattern language_alias();
        inline textx::arpeggio::Pattern ident();
        inline textx::arpeggio::Pattern rule_name();
        inline textx::arpeggio::Pattern rule_params();
        inline textx::arpeggio::Pattern textx_rule_body();
        inline textx::arpeggio::Pattern rule_param();
        inline textx::arpeggio::Pattern param_name();
        inline textx::arpeggio::Pattern string_value();
        inline textx::arpeggio::Pattern choice();
        inline textx::arpeggio::Pattern sequence();
        inline textx::arpeggio::Pattern repeatable_expr();
        inline textx::arpeggio::Pattern expression();
        inline textx::arpeggio::Pattern repeat_operator();
        inline textx::arpeggio::Pattern assignment();
        inline textx::arpeggio::Pattern syntactic_predicate();
        inline textx::arpeggio::Pattern simple_match();
        inline textx::arpeggio::Pattern rule_ref();
        inline textx::arpeggio::Pattern bracketed_choice();
        inline textx::arpeggio::Pattern repeat_modifiers();
        inline textx::arpeggio::Pattern str_match();
        inline textx::arpeggio::Pattern re_match();
        inline textx::arpeggio::Pattern attribute();
        inline textx::arpeggio::Pattern assignment_op();
        inline textx::arpeggio::Pattern assignment_rhs();
        inline textx::arpeggio::Pattern reference();
        inline textx::arpeggio::Pattern obj_ref();
        inline textx::arpeggio::Pattern class_name();
        inline textx::arpeggio::Pattern obj_ref_rule();
        inline textx::arpeggio::Pattern rrel_expression();
        inline textx::arpeggio::Pattern qualified_ident();
        inline textx::arpeggio::Pattern comment_line();
        inline textx::arpeggio::Pattern comment_block();
        inline textx::arpeggio::Pattern rrel_sequence();

        // textX grammar
        inline textx::arpeggio::Pattern textx_model()
        {
            return textx::arpeggio::sequence({textx::arpeggio::zero_or_more(import_or_reference_stm()),
                                              textx::arpeggio::zero_or_more(textx_rule())});
        }

        inline textx::arpeggio::Pattern import_or_reference_stm()
        {
            return textx::arpeggio::ordered_choice({import_stm(),
                                                    reference_stm()});
        }

        inline textx::arpeggio::Pattern import_stm()
        {
            return textx::arpeggio::sequence({textx::arpeggio::str_match("import"),
                                              grammar_to_import()});
        }

        inline textx::arpeggio::Pattern reference_stm()
        {
            return textx::arpeggio::sequence({textx::arpeggio::str_match("reference"),
                                              language_name(),
                                              textx::arpeggio::optional(language_alias())});
        }

        inline textx::arpeggio::Pattern language_alias()
        {
            return textx::arpeggio::sequence({textx::arpeggio::str_match("as"),
                                              ident()});
        }

        inline textx::arpeggio::Pattern language_name()
        {
            return textx::arpeggio::regex_match(R"((\w|-)+)");
        }

        inline textx::arpeggio::Pattern grammar_to_import()
        {
            return textx::arpeggio::regex_match(R"((\w|\.)+)");
        };

        // Rules
        inline textx::arpeggio::Pattern textx_rule()
        {
            return textx::arpeggio::sequence({rule_name(),
                                              textx::arpeggio::optional(rule_params()),
                                              textx::arpeggio::str_match(":"),
                                              textx_rule_body(),
                                              textx::arpeggio::str_match(";")});
        }

        inline textx::arpeggio::Pattern rule_params()
        {
            return textx::arpeggio::sequence({textx::arpeggio::str_match("["),
                                              rule_param(),
                                              textx::arpeggio::zero_or_more(textx::arpeggio::sequence({textx::arpeggio::str_match(","),
                                                                                                       rule_param()})),
                                              textx::arpeggio::str_match("]")});
        }

        inline textx::arpeggio::Pattern rule_param()
        {
            return textx::arpeggio::sequence({param_name(),
                                              textx::arpeggio::optional(textx::arpeggio::sequence({textx::arpeggio::str_match("="),
                                                                                                   string_value()}))});
        }

        inline textx::arpeggio::Pattern param_name()
        {
            return ident();
        };

        inline textx::arpeggio::Pattern textx_rule_body()
        {
            return choice();
        }

        inline textx::arpeggio::Pattern choice()
        {
            return textx::arpeggio::sequence({sequence(), textx::arpeggio::zero_or_more(
                                                              textx::arpeggio::sequence({textx::arpeggio::str_match("|"),
                                                                                         sequence()}))});
        }

        inline textx::arpeggio::Pattern sequence()
        {
            return textx::arpeggio::one_or_more(repeatable_expr());
        }

        inline textx::arpeggio::Pattern repeatable_expr()
        {
            return textx::arpeggio::sequence({expression(),
                                              textx::arpeggio::optional(repeat_operator()),
                                              textx::arpeggio::optional(textx::arpeggio::str_match("-"))});
        }

        inline textx::arpeggio::Pattern expression()
        {
            return textx::arpeggio::ordered_choice({assignment(),
                                                    textx::arpeggio::sequence({textx::arpeggio::optional(syntactic_predicate()),
                                                                               textx::arpeggio::ordered_choice({simple_match(),
                                                                                                                rule_ref(),
                                                                                                                bracketed_choice()})})});
        }

        inline textx::arpeggio::Pattern bracketed_choice()
        {
            return textx::arpeggio::sequence({textx::arpeggio::str_match("("),
                                              choice(),
                                              textx::arpeggio::str_match(")")});
        }

        inline textx::arpeggio::Pattern repeat_operator()
        {
            return textx::arpeggio::sequence({textx::arpeggio::ordered_choice({textx::arpeggio::str_match("*"),
                                                                               textx::arpeggio::str_match("?"),
                                                                               textx::arpeggio::str_match("+"),
                                                                               textx::arpeggio::str_match("#")}),
                                              textx::arpeggio::optional(repeat_modifiers())});
        }

        inline textx::arpeggio::Pattern repeat_modifiers()
        {
            return textx::arpeggio::sequence({textx::arpeggio::str_match("["),
                                              textx::arpeggio::one_or_more(
                                                  textx::arpeggio::ordered_choice({simple_match(),
                                                                                   textx::arpeggio::str_match("eolterm")})),
                                              textx::arpeggio::str_match("]")});
        }

        inline textx::arpeggio::Pattern syntactic_predicate()
        {
            return textx::arpeggio::ordered_choice({textx::arpeggio::str_match("!"),
                                                    textx::arpeggio::str_match("&")});
        }

        inline textx::arpeggio::Pattern simple_match()
        {
            return textx::arpeggio::ordered_choice({str_match(),
                                                    re_match()});
        }

        // Assignment
        inline textx::arpeggio::Pattern assignment()
        {
            return textx::arpeggio::sequence({attribute(),
                                              assignment_op(),
                                              assignment_rhs()});
        }

        inline textx::arpeggio::Pattern attribute()
        {
            return ident();
        }

        inline textx::arpeggio::Pattern assignment_op()
        {
            return textx::arpeggio::ordered_choice({textx::arpeggio::str_match("="),
                                                    textx::arpeggio::str_match("*="),
                                                    textx::arpeggio::str_match("+="),
                                                    textx::arpeggio::str_match("?=")});
        }

        inline textx::arpeggio::Pattern assignment_rhs()
        {
            return textx::arpeggio::sequence({textx::arpeggio::ordered_choice({simple_match(),
                                                                               reference()}),
                                              textx::arpeggio::optional(repeat_modifiers())});
        }

        // References
        inline textx::arpeggio::Pattern reference()
        {
            return textx::arpeggio::ordered_choice({rule_ref(),
                                                    obj_ref()});
        }

        inline textx::arpeggio::Pattern rule_ref()
        {
            return ident();
        }

        inline textx::arpeggio::Pattern obj_ref()
        {
            return textx::arpeggio::sequence({
                textx::arpeggio::str_match("["),
                class_name(),
                textx::arpeggio::optional(textx::arpeggio::sequence({
                    textx::arpeggio::str_match("|"),
                    obj_ref_rule(), 
                    textx::arpeggio::optional(textx::arpeggio::sequence({
                        textx::arpeggio::str_match("|"),
                        rrel_expression()
                    }))
                })),
                textx::arpeggio::str_match("]")
            });
        }

        inline textx::arpeggio::Pattern rule_name()
        {
            return ident();
        }

        inline textx::arpeggio::Pattern obj_ref_rule()
        {
            return ident();
        }

        inline textx::arpeggio::Pattern class_name()
        {
            return qualified_ident();
        }

        inline textx::arpeggio::Pattern str_match()
        {
            return string_value();
        }

        inline textx::arpeggio::Pattern re_match()
        {
            return textx::arpeggio::regex_match(R"(/((?:(?:\\/)|[^/])*)/)");
        }

        inline textx::arpeggio::Pattern ident()
        {
            return textx::arpeggio::regex_match(R"(\w+)");
        }

        inline textx::arpeggio::Pattern qualified_ident()
        {
            return textx::arpeggio::regex_match(R"(\w+(\.\w+)?)");
        }

        inline textx::arpeggio::Pattern integer()
        {
            return textx::arpeggio::regex_match(R"([-+]?[0-9]+)");
        }

        inline textx::arpeggio::Pattern string_value()
        {
            return textx::arpeggio::ordered_choice({textx::arpeggio::regex_match(R"('((\\')|[^'])*')"),
                                                    textx::arpeggio::regex_match(R"("((\\")|[^"])*")")});
        }

        // Comments
        inline textx::arpeggio::Pattern comment()
        {
            return textx::arpeggio::ordered_choice({comment_line(),
                                                    comment_block()});
        }

        inline textx::arpeggio::Pattern comment_line()
        {
            return textx::arpeggio::regex_match(R"(//.*?$)");
        }

        inline textx::arpeggio::Pattern comment_block()
        {
            return textx::arpeggio::regex_match(R"(/\*(.|\n)*?\*/)");
        }

        inline textx::arpeggio::Pattern rrel_id() {
            return textx::arpeggio::regex_match(R"([^\d\W]\w*\b)");  // from lang.py
        }

        inline textx::arpeggio::Pattern rrel_parent() {
            return textx::arpeggio::sequence({
                textx::arpeggio::str_match("parent"),
                textx::arpeggio::str_match("("), 
                rrel_id(),
                textx::arpeggio::str_match(")")
            });
        }

        inline textx::arpeggio::Pattern rrel_navigation() {
            return textx::arpeggio::sequence({
                textx::arpeggio::optional(textx::arpeggio::str_match("~")), 
                rrel_id()
            });
        }

        inline textx::arpeggio::Pattern rrel_brackets() {
            return textx::arpeggio::sequence({
                textx::arpeggio::str_match("("), 
                rrel_sequence(),
                textx::arpeggio::str_match(")")
            });
        }

        inline textx::arpeggio::Pattern rrel_dots() {
            return textx::arpeggio::regex_match(R"(\.+)");
        }

        inline textx::arpeggio::Pattern rrel_path_element() {
            return textx::arpeggio::ordered_choice({
                rrel_parent(),
                rrel_brackets(),
                rrel_navigation()
            });
        }

        inline textx::arpeggio::Pattern rrel_zero_or_more() {
            return textx::arpeggio::sequence({
                rrel_path_element(),
                textx::arpeggio::str_match("*")
            });
        }

        inline textx::arpeggio::Pattern rrel_path() {
            return textx::arpeggio::sequence({
                textx::arpeggio::optional(textx::arpeggio::sequence({
                    textx::arpeggio::str_match("^"), 
                    rrel_dots()
                })), 
                textx::arpeggio::zero_or_more(
                    textx::arpeggio::sequence({
                        textx::arpeggio::ordered_choice({rrel_zero_or_more(), rrel_path_element()}), 
                        textx::arpeggio::str_match(".")
                    })
                ), 
                textx::arpeggio::optional( // echt???
                    textx::arpeggio::ordered_choice({
                        rrel_zero_or_more(), 
                        rrel_path_element()
                    })
                )
            });
        }

        inline textx::arpeggio::Pattern rrel_sequence() {
            return textx::arpeggio::zero_or_more(textx::arpeggio::sequence({
                rrel_path(),
                textx::arpeggio::str_match(","),
                rrel_path()
            }));
        }

        inline textx::arpeggio::Pattern rrel_expression() {
            return textx::arpeggio::sequence({
                textx::arpeggio::optional(textx::arpeggio::regex_match(R"(\+[mp]+:)")),
                rrel_sequence()
            });
        }

    }
}
