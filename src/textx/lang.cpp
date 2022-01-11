#include "textx/arpeggio.h"
#include "textx/lang.h"

namespace textx
{
    namespace lang
    {
        TextxGrammar::TextxGrammar()
        {
            namespace ta = textx::arpeggio;

            // textX grammar
            set_main_rule("textx_model");
            get_config().skip_text = textx::arpeggio::skip_text_functions::skip_cpp_style();

            add_rule("textx_model", ta::sequence({ta::zero_or_more(ref("import_or_reference_stm")),
                                                  ta::zero_or_more(ref("textx_rule")), ta::end_of_file()}));

            add_rule("import_or_reference_stm", ta::ordered_choice({ref("import_stm"),
                                                                    ref("reference_stm")}));

            add_rule("import_stm", ta::sequence({ta::str_match("import"),
                                                 ref("grammar_to_import")}));

            add_rule("reference_stm", ta::sequence({ta::str_match("reference"),
                                                    ref("language_name"),
                                                    ta::optional(ref("language_alias"))}));

            add_rule("language_alias", ta::sequence({ta::str_match("as"),
                                                     ref("ident")}));

            add_rule("language_name", ta::capture(ta::regex_match(R"((\w|-)+)")));

            add_rule("grammar_to_import", ta::capture(ta::regex_match(R"((\w|\.)+)")));

            // Rules
            add_rule("textx_rule", ta::sequence({ref("rule_name"),
                                                 ta::optional(ref("rule_params")),
                                                 ta::str_match(":"),
                                                 ref("textx_rule_body"),
                                                 ta::str_match(";")}));

            add_rule("rule_params", ta::sequence({ta::str_match("["),
                                                  ref("rule_param"),
                                                  ta::zero_or_more(ta::sequence({ta::str_match(","),
                                                                                 ref("rule_param")})),
                                                  ta::str_match("]")}));

            add_rule("rule_param", ta::sequence({ref("param_name"),
                                                 ta::optional(ta::sequence({ta::str_match("="),
                                                                            ref("string_value")}))}));

            add_rule("param_name", ref("ident"));

            add_rule("textx_rule_body", copy("choice"));

            add_rule("choice", ta::sequence({ref("sequence"), ta::zero_or_more(
                                                                  ta::sequence({ta::str_match("|"),
                                                                                ref("sequence")}))}));

            add_rule("sequence", ta::one_or_more(ref("repeatable_expr")));

            add_rule("repeatable_expr", ta::sequence({ref("expression"),
                                                      ta::capture(ta::optional(ref("repeat_operator"))),
                                                      ta::capture(ta::optional(ta::str_match("-")))}));

            add_rule("expression", ta::ordered_choice({ref("assignment"),
                                                       ta::sequence({ta::optional(ref("syntactic_predicate")),
                                                                     ta::ordered_choice({ref("simple_match"),
                                                                                         ref("rule_ref"),
                                                                                         ref("bracketed_choice")})})}));

            add_rule("bracketed_choice", ta::sequence({ta::str_match("("),
                                                       ref("choice"),
                                                       ta::str_match(")")}));

            add_rule("repeat_operator", ta::sequence({ref("repeat_operator_text"),
                                                      ta::optional(ref("repeat_modifiers"))}));
            add_rule("repeat_operator_text",ta::capture(ta::ordered_choice({ta::str_match("*"),
                                                                            ta::str_match("?"),
                                                                            ta::str_match("+"),
                                                                            ta::str_match("#")})));

            add_rule("repeat_modifiers", ta::sequence({ta::str_match("["),
                                                       ta::capture(ta::one_or_more(
                                                           ta::ordered_choice({ref("simple_match"),
                                                                               ta::str_match("eolterm")}))),
                                                       ta::str_match("]")}));

            add_rule("syntactic_predicate", ta::capture(ta::ordered_choice({ta::str_match("!"),
                                                                            ta::str_match("&")})));

            add_rule("simple_match", ta::ordered_choice({ref("str_match"),
                                                         ref("re_match")}));

            // Assignment
            add_rule("assignment", ta::sequence({ref("attribute"),
                                                 ref("assignment_op"),
                                                 ref("assignment_rhs")}));

            add_rule("attribute", ref("ident"));

            add_rule("assignment_op", ta::capture(ta::ordered_choice({ta::str_match("="),
                                                                      ta::str_match("*="),
                                                                      ta::str_match("+="),
                                                                      ta::str_match("?=")})));

            add_rule("assignment_rhs", ta::sequence({ta::ordered_choice({ref("simple_match"),
                                                                         ref("reference")}),
                                                     ta::optional(ref("repeat_modifiers"))}));

            // References
            add_rule("reference", ta::ordered_choice({ref("rule_ref"),
                                                      ref("obj_ref")}));

            add_rule("rule_ref", ta::capture(ref("ident")));

            add_rule("obj_ref", ta::sequence({ta::str_match("["),
                                              ta::capture(ref("class_name")),
                                              ta::optional(ta::sequence({ta::str_match("|"),
                                                                         ref("obj_ref_rule"),
                                                                         ta::optional(ta::sequence({ta::str_match("|"),
                                                                                                    ref("rrel_expression")}))})),
                                              ta::str_match("]")}));

            add_rule("rule_name", ref("ident"));

            add_rule("obj_ref_rule", ref("ident"));

            add_rule("class_name", ref("qualified_ident"));

            add_rule("str_match", ref("string_value"));

            add_rule("re_match", ta::capture(ta::regex_match(R"(/((?:(?:\\/)|[^/])*)/)")));

            add_rule("ident", ta::capture(ta::regex_match(R"(\w+)")));

            add_rule("qualified_ident", ta::capture(ta::regex_match(R"(\w+(\.\w+)?)")));

            add_rule("integer", ta::capture(ta::regex_match(R"([-+]?[0-9]+)")));

            add_rule("string_value", ta::capture(ta::ordered_choice({ta::regex_match(R"('((\\')|[^'])*')"),
                                                         ta::regex_match(R"("((\\")|[^"])*")")})));

            // Comments
            // add_rule("comment", ta::ordered_choice({ref("comment_line"),
            //                             ref("comment_block")}));

            // add_rule("comment_line", ta::regex_match(R"(//.*?$)"));

            // add_rule("comment_block", ta::regex_match(R"(/\*(.|\n)*?\*/)"));
            // see above skip_cpp_style

            add_rule("rrel_id", ta::capture(ta::regex_match(R"([^\d\W]\w*\b)"))); // from lang.py

            add_rule("rrel_parent", ta::sequence({ta::str_match("parent"),
                                                  ta::str_match("("),
                                                  ref("rrel_id"),
                                                  ta::str_match(")")}));

            add_rule("rrel_navigation", ta::sequence({ta::optional(ta::sequence({
                                                        ta::capture(ta::optional(ref("string_value"))),
                                                        ta::capture(ta::str_match("~"))
                                                      })),
                                                      ref("rrel_id")}));

            add_rule("rrel_brackets", ta::sequence({ta::str_match("("),
                                                    ref("rrel_sequence"),
                                                    ta::str_match(")")}));

            add_rule("rrel_dots", ta::capture(ta::regex_match(R"(\.+)")));

            add_rule("rrel_path_element", ta::ordered_choice({ref("rrel_parent"),
                                                              ref("rrel_brackets"),
                                                              ref("rrel_navigation")}));

            add_rule("rrel_zero_or_more", ta::sequence({ref("rrel_path_element"),
                                                        ta::str_match("*")}));

            add_rule("rrel_path", ta::ordered_choice({ta::sequence({ta::optional(ta::ordered_choice({ta::capture(ta::str_match("^")),
                                                                                                     ref("rrel_dots")})),
                                                                    ta::zero_or_more(
                                                                        ta::sequence({ta::ordered_choice({ref("rrel_zero_or_more"), ref("rrel_path_element")}),
                                                                                      ta::str_match(".")})),
                                                                    ta::ordered_choice({ref("rrel_zero_or_more"),
                                                                                        ref("rrel_path_element")})}),
                                                      ta::ordered_choice({ta::capture(ta::str_match("^")),
                                                                          ref("rrel_dots")})}));

            add_rule("rrel_sequence", ta::sequence({ta::zero_or_more(ta::sequence({ref("rrel_path"),
                                                                                   ta::str_match(",")})),
                                                    ref("rrel_path")}));

            add_rule("rrel_options", ta::capture(ta::regex_match(R"(\+[mp]+:)")));

            add_rule("rrel_expression", ta::sequence({ta::optional(ref("rrel_options")),
                                                      ref("rrel_sequence")}));
        }
    }
}