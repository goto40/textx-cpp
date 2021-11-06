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

            add_rule("textx_model", ta::sequence({ta::zero_or_more(ref("import_or_reference_stm")),
                                                  ta::zero_or_more(ref("textx_rule"))}));


        }
    }
}