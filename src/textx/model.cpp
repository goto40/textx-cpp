#include "textx/model.h"
#include "textx/metamodel.h"
#include "textx/arpeggio.h"

namespace textx {

    Model::Model(const std::string_view text, const textx::arpeggio::Match &parsetree, std::shared_ptr<Metamodel> mm) : weak_mm{mm}, root(create_model(text, parsetree, *mm)) {
    } 

    textx::object::Value Model::create_model(const std::string_view text, const textx::arpeggio::Match &m, textx::Metamodel &mm) {
         if (m.name.has_value() && m.name.value().starts_with("rule://")) {
            std::string rule_name = m.name.value().substr(7);
            auto &rule = mm[rule_name];
            if (rule.type() == RuleType::match) {
                return std::string(textx::arpeggio::get_str(text, m));
            }
            else {
                textx::arpeggio::raise(m.start(), "TODO");
            }
        }
        else {
            textx::arpeggio::raise(m.start(), "unexpected, no rule result found here...");
        }
    }

}