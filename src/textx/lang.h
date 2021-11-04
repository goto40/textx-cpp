#pragma once;
#include "textx/arpeggio.h"

namespace textx
{
    /** adapted grammar from lang.py */
    namespace lang
    {
        // textX grammar
        inline auto textx_model()
        {
            return textx::apreggio::sequence({textx::apreggio::zero_or_more(import_or_reference_stm()),
                                              textx::apreggio::zero_or_more(textx_rule())});
        }

        inline auto import_or_reference_stm()
        {
            return textx::arpeggio::ordered_choice({import_stm(),
                                                    reference_stm()});
        }

        inline auto import_stm()
        {
            return textx::apreggio::sequence({textx::apreggio::str_match("import"),
                                              grammar_to_import()});
        }

        inline auto reference_stm()
        {
            return textx::apreggio::sequence({textx::apreggio::str_match("reference"),
                                              language_name(),
                                              textx::apreggio::optional(language_alias())});
        }

        inline auto language_alias()
        {
            return textx::apreggio::sequence({textx::apreggio::str_match("as"),
                                              ident()});
        }

        inline auto language_name()
        {
            return textx::apreggio::regex_match(R"((\w|-)+)");
        }

        inline auto grammar_to_import()
        {
            return textx::apreggio::regex_match(R"((\w|\.)+)");
        };

        // Rules
        inline auto textx_rule()
        {
            return textx::apreggio::sequence({rule_name(),
                                              textx::apreggio::optional(rule_params()),
                                              textx::apreggio::str_match(":"),
                                              textx_rule_body(),
                                              textx::apreggio::str_match(";")});
        }

        inline auto rule_params()
        {
            return textx::apreggio::sequence({textx::apreggio::str_match("["),
                                              rule_param(),
                                              textx::apreggio::zero_or_more({textx::apreggio::str_match(","),
                                                                             rule_param()}),
                                              textx::apreggio::str_match("]")});
        }

        inline auto rule_param()
        {
            return textx::apreggio::sequence({param_name(),
                                              textx::apreggio::optional(textx::apreggio::sequence({textx::apreggio::str_match("="),
                                                                                                   string_value()}))});
        }

        inline auto param_name()
        {
            return ident();
        };

        inline auto textx_rule_body()
        {
            return choice();
        }

        inline auto choice()
        {
            return textx::apreggio::sequence({sequence(), textx::apreggio::zero_or_more(
                                                              textx::apreggio::sequence({textx::apreggio::str_match("|"),
                                                                                         sequence()}))});
        }

        auto sequence()
        {
            return textx::apreggio::one_or_more(repeatable_expr());
        }

        auto repeatable_expr()
        {
            return textx::apreggio::sequence({expression(),
                                              textx::apreggio::optional(repeat_operator()),
                                              textx::apreggio::optional(textx::apreggio::str_match("-"))});
        }

        auto expression()
        {
            return textx::apreggio::ordered_choice({assignment(),
                                                    textx::apreggio::sequence({textx::apreggio::optional(syntactic_predicate()),
                                                                               textx::apreggio::ordered_choice({simple_match(),
                                                                                                                rule_ref(),
                                                                                                                bracketed_choice()})})});
        }

        auto bracketed_choice()
        {
            return textx::apreggio::sequence({textx::apreggio::str_match("("),
                                              choice(),
                                              textx::apreggio::str_match(")")});
        }

        auto repeat_operator()
        {
            return textx::apreggio::sequence({textx::apreggio::ordered_choice({textx::apreggio::str_match("*"),
                                                                               textx::apreggio::str_match("?"),
                                                                               textx::apreggio::str_match("+"),
                                                                               textx::apreggio::str_match("#")}),
                                              textx::apreggio::optional(repeat_modifiers())});
        }

        auto repeat_modifiers()
        {
            return textx::apreggio::sequence({textx::apreggio::str_match("["),
                                              textx::apreggio::one_or_more(
                                                  textx::apreggio::ordered_choice({simple_match(),
                                                                                   textx::apreggio::str_match("eolterm")})),
                                              textx::apreggio::str_match("]")});
        }

        auto syntactic_predicate()
        {
            return textx::apreggio::ordered_choice({textx::apreggio::str_match("!"),
                                                    textx::apreggio::str_match("&")});
        }

        auto simple_match()
        {
            return textx::apreggio::ordered_choice({str_match(),
                                                    re_match()});
        }

        // Assignment
        auto assignment()
        {
            return return textx::apreggio::sequence({attribute(),
                                                     assignment_op(),
                                                     assignment_rhs()});
        }

        auto attribute()
        {
            return ident();
        }

        auto assignment_op()
        {
            return textx::apreggio::ordered_choice({textx::apreggio::str_match("="),
                                                    textx::apreggio::str_match("*="),
                                                    textx::apreggio::str_match("+="),
                                                    textx::apreggio::str_match("?=")});
        }

        auto assignment_rhs()
        {
            return textx::apreggio::sequence({textx::apreggio::ordered_choice({simple_match(),
                                                                               reference()}),
                                              textx::apreggio::optional(repeat_modifiers())});
        }

        // References
        auto reference()
        {
            return textx::apreggio::ordered_choice({rule_ref(),
                                                    obj_ref()});
        }

        auto rule_ref()
        {
            return ident();
        }

        auto obj_ref()
        {
            return textx::apreggio::sequence({
                textx::apreggio::str_match("["),
                class_name(),
                textx::apreggio::optional(textx::apreggio::sequence({
                    textx::apreggio::str_match("|")),
                    obj_ref_rule(), 
                    textx::apreggio::optional(textx::apreggio::sequence({
                        textx::apreggio::str_match("|"),
                        rrel_expression()
                    }))
                }),
                textx::apreggio::str_match("]")
            });
        }

        auto rule_name() {
            return ident();
        }

        auto obj_ref_rule() {
            return ident();
        }

        auto class_name() {
            return qualified_ident();
        }

        autp str_match() {
            return string_value();
        }

        autp re_match() {
            return textx::apreggio::regex_match(R"(/((?:(?:\\/)|[^/])*)/)");
        }

        auto ident() {
            return textx::apreggio::regex_match(R"(\w+)");
        }

        auto qualified_ident() {
            return textx::apreggio::regex_match(R"(\w+(\.\w+)?)");
        }

        autp integer() {
            return textx::apreggio::regex_match(R"([-+]?[0-9]+)");
        }

        auto string_value() {
            return textx::apreggio::ordered_choice({
                textx::apreggio::regex_match(R"('((\\')|[^'])*')"),
                textx::apreggio::regex_match(R"("((\\")|[^"])*")")
            });
        }

        // Comments
        auto comment() {
            return textx::apreggio::ordered_choice({
                comment_line(),
                comment_block()
            });
        }

        auto comment_line() {
            return textx::apreggio::regex_match(R"(//.*?$)");
        }

        def comment_block() {
            return textx::apreggio::regex_match(R"(/\*(.|\n)*?\*/)");
        }

    }