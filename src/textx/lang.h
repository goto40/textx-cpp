#pragma once
#include "textx/arpeggio.h"

namespace textx
{
    /** adapted grammar from lang.py */
    namespace lang
    {
        namespace ta = textx::arpeggio;

        inline ta::Pattern import_or_reference_stm();
        inline ta::Pattern import_stm();
        inline ta::Pattern reference_stm();
        inline ta::Pattern textx_rule();
        inline ta::Pattern grammar_to_import();
        inline ta::Pattern language_name();
        inline ta::Pattern language_alias();
        inline ta::Pattern ident();
        inline ta::Pattern rule_name();
        inline ta::Pattern rule_params();
        inline ta::Pattern textx_rule_body();
        inline ta::Pattern rule_param();
        inline ta::Pattern param_name();
        inline ta::Pattern string_value();
        inline ta::Pattern choice();
        inline ta::Pattern sequence();
        inline ta::Pattern repeatable_expr();
        inline ta::Pattern expression();
        inline ta::Pattern repeat_operator();
        inline ta::Pattern assignment();
        inline ta::Pattern syntactic_predicate();
        inline ta::Pattern simple_match();
        inline ta::Pattern rule_ref();
        inline ta::Pattern bracketed_choice();
        inline ta::Pattern repeat_modifiers();
        inline ta::Pattern str_match();
        inline ta::Pattern re_match();
        inline ta::Pattern attribute();
        inline ta::Pattern assignment_op();
        inline ta::Pattern assignment_rhs();
        inline ta::Pattern reference();
        inline ta::Pattern obj_ref();
        inline ta::Pattern class_name();
        inline ta::Pattern obj_ref_rule();
        inline ta::Pattern rrel_expression();
        inline ta::Pattern qualified_ident();
        inline ta::Pattern comment_line();
        inline ta::Pattern comment_block();
        inline ta::Pattern rrel_sequence();

        // textX grammar
        inline ta::Pattern textx_model()
        {
            return ta::sequence({ta::zero_or_more(import_or_reference_stm()),
                                 ta::zero_or_more(textx_rule())});
        }

        inline ta::Pattern import_or_reference_stm()
        {
            return ta::ordered_choice({import_stm(),
                                       reference_stm()});
        }

        inline ta::Pattern import_stm()
        {
            return ta::sequence({ta::str_match("import"),
                                 grammar_to_import()});
        }

        inline ta::Pattern reference_stm()
        {
            return ta::sequence({ta::str_match("reference"),
                                 language_name(),
                                 ta::optional(language_alias())});
        }

        inline ta::Pattern language_alias()
        {
            return ta::sequence({ta::str_match("as"),
                                 ident()});
        }

        inline ta::Pattern language_name()
        {
            return ta::regex_match(R"((\w|-)+)");
        }

        inline ta::Pattern grammar_to_import()
        {
            return ta::regex_match(R"((\w|\.)+)");
        };

        // Rules
        inline ta::Pattern textx_rule()
        {
            return ta::sequence({rule_name(),
                                 ta::optional(rule_params()),
                                 ta::str_match(":"),
                                 textx_rule_body(),
                                 ta::str_match(";")});
        }

        inline ta::Pattern rule_params()
        {
            return ta::sequence({ta::str_match("["),
                                 rule_param(),
                                 ta::zero_or_more(ta::sequence({ta::str_match(","),
                                                                rule_param()})),
                                 ta::str_match("]")});
        }

        inline ta::Pattern rule_param()
        {
            return ta::sequence({param_name(),
                                 ta::optional(ta::sequence({ta::str_match("="),
                                                            string_value()}))});
        }

        inline ta::Pattern param_name()
        {
            return ident();
        };

        inline ta::Pattern textx_rule_body()
        {
            return choice();
        }

        inline ta::Pattern choice()
        {
            return ta::sequence({sequence(), ta::zero_or_more(
                                                 ta::sequence({ta::str_match("|"),
                                                               sequence()}))});
        }

        inline ta::Pattern sequence()
        {
            return ta::one_or_more(repeatable_expr());
        }

        inline ta::Pattern repeatable_expr()
        {
            return ta::sequence({expression(),
                                 ta::optional(repeat_operator()),
                                 ta::optional(ta::str_match("-"))});
        }

        inline ta::Pattern expression()
        {
            return ta::ordered_choice({assignment(),
                                       ta::sequence({ta::optional(syntactic_predicate()),
                                                     ta::ordered_choice({simple_match(),
                                                                         rule_ref(),
                                                                         bracketed_choice()})})});
        }

        inline ta::Pattern bracketed_choice()
        {
            return ta::sequence({ta::str_match("("),
                                 choice(),
                                 ta::str_match(")")});
        }

        inline ta::Pattern repeat_operator()
        {
            return ta::sequence({ta::ordered_choice({ta::str_match("*"),
                                                     ta::str_match("?"),
                                                     ta::str_match("+"),
                                                     ta::str_match("#")}),
                                 ta::optional(repeat_modifiers())});
        }

        inline ta::Pattern repeat_modifiers()
        {
            return ta::sequence({ta::str_match("["),
                                 ta::one_or_more(
                                     ta::ordered_choice({simple_match(),
                                                         ta::str_match("eolterm")})),
                                 ta::str_match("]")});
        }

        inline ta::Pattern syntactic_predicate()
        {
            return ta::ordered_choice({ta::str_match("!"),
                                       ta::str_match("&")});
        }

        inline ta::Pattern simple_match()
        {
            return ta::ordered_choice({str_match(),
                                       re_match()});
        }

        // Assignment
        inline ta::Pattern assignment()
        {
            return ta::sequence({attribute(),
                                 assignment_op(),
                                 assignment_rhs()});
        }

        inline ta::Pattern attribute()
        {
            return ident();
        }

        inline ta::Pattern assignment_op()
        {
            return ta::ordered_choice({ta::str_match("="),
                                       ta::str_match("*="),
                                       ta::str_match("+="),
                                       ta::str_match("?=")});
        }

        inline ta::Pattern assignment_rhs()
        {
            return ta::sequence({ta::ordered_choice({simple_match(),
                                                     reference()}),
                                 ta::optional(repeat_modifiers())});
        }

        // References
        inline ta::Pattern reference()
        {
            return ta::ordered_choice({rule_ref(),
                                       obj_ref()});
        }

        inline ta::Pattern rule_ref()
        {
            return ident();
        }

        inline ta::Pattern obj_ref()
        {
            return ta::sequence({ta::str_match("["),
                                 class_name(),
                                 ta::optional(ta::sequence({ta::str_match("|"),
                                                            obj_ref_rule(),
                                                            ta::optional(ta::sequence({ta::str_match("|"),
                                                                                       rrel_expression()}))})),
                                 ta::str_match("]")});
        }

        inline ta::Pattern rule_name()
        {
            return ident();
        }

        inline ta::Pattern obj_ref_rule()
        {
            return ident();
        }

        inline ta::Pattern class_name()
        {
            return qualified_ident();
        }

        inline ta::Pattern str_match()
        {
            return string_value();
        }

        inline ta::Pattern re_match()
        {
            return ta::regex_match(R"(/((?:(?:\\/)|[^/])*)/)");
        }

        inline ta::Pattern ident()
        {
            return ta::regex_match(R"(\w+)");
        }

        inline ta::Pattern qualified_ident()
        {
            return ta::regex_match(R"(\w+(\.\w+)?)");
        }

        inline ta::Pattern integer()
        {
            return ta::regex_match(R"([-+]?[0-9]+)");
        }

        inline ta::Pattern string_value()
        {
            return ta::ordered_choice({ta::regex_match(R"('((\\')|[^'])*')"),
                                       ta::regex_match(R"("((\\")|[^"])*")")});
        }

        // Comments
        inline ta::Pattern comment()
        {
            return ta::ordered_choice({comment_line(),
                                       comment_block()});
        }

        inline ta::Pattern comment_line()
        {
            return ta::regex_match(R"(//.*?$)");
        }

        inline ta::Pattern comment_block()
        {
            return ta::regex_match(R"(/\*(.|\n)*?\*/)");
        }

        inline ta::Pattern rrel_id()
        {
            return ta::regex_match(R"([^\d\W]\w*\b)"); // from lang.py
        }

        inline ta::Pattern rrel_parent()
        {
            return ta::sequence({ta::str_match("parent"),
                                 ta::str_match("("),
                                 rrel_id(),
                                 ta::str_match(")")});
        }

        inline ta::Pattern rrel_navigation()
        {
            return ta::sequence({ta::optional(ta::str_match("~")),
                                 rrel_id()});
        }

        inline ta::Pattern rrel_brackets()
        {
            return ta::sequence({ta::str_match("("),
                                 rrel_sequence(),
                                 ta::str_match(")")});
        }

        inline ta::Pattern rrel_dots()
        {
            return ta::regex_match(R"(\.+)");
        }

        inline ta::Pattern rrel_path_element()
        {
            return ta::ordered_choice({rrel_parent(),
                                       rrel_brackets(),
                                       rrel_navigation()});
        }

        inline ta::Pattern rrel_zero_or_more()
        {
            return ta::sequence({rrel_path_element(),
                                 ta::str_match("*")});
        }

        inline ta::Pattern rrel_path()
        {
            return ta::sequence({ta::optional(ta::sequence({ta::str_match("^"),
                                                            rrel_dots()})),
                                 ta::zero_or_more(
                                     ta::sequence({ta::ordered_choice({rrel_zero_or_more(), rrel_path_element()}),
                                                   ta::str_match(".")})),
                                 ta::optional( // echt???
                                     ta::ordered_choice({rrel_zero_or_more(),
                                                         rrel_path_element()}))});
        }

        inline ta::Pattern rrel_sequence()
        {
            return ta::zero_or_more(ta::sequence({rrel_path(),
                                                  ta::str_match(","),
                                                  rrel_path()}));
        }

        inline ta::Pattern rrel_expression()
        {
            return ta::sequence({ta::optional(ta::regex_match(R"(\+[mp]+:)")),
                                 rrel_sequence()});
        }

    }
}
