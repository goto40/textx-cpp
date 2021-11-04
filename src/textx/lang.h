#pragma once;
#include "textx/arpeggio.h"

namespace textx
{
    namespace lang
    {
        // textX grammar
        auto textx_model()
        {
            return textx::apreggio::sequence({
                textx::apreggio::zero_or_more(import_or_reference_stm()),
                textx::apreggio::zero_or_more(textx_rule())
            });
        }
        auto import_or_reference_stm()
        {
            return textx::arpeggio::ordered_choice({
                import_stm(),
                reference_stm()
            });
        }

        auto import_stm()
        {
            return textx::apreggio::sequence({
                textx::apreggio::str_match("import"),
                grammar_to_import()
            });
        }

        auto reference_stm()
        {
            return textx::apreggio::sequence({
                textx::apreggio::str_match("reference"), 
                language_name(),
                textx::apreggio::optional(language_alias())
            });
        }
            /*def language_alias():       return 'as', ident
            def language_name():        return _(r'(\w|-)+')
            def grammar_to_import():    return _(r'(\w|\.)+')

            # Rules
            def textx_rule():           return rule_name, Optional(rule_params), ":", textx_rule_body, ";"
            */
    }