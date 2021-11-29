#include "textx/model.h"
#include "textx/metamodel.h"
#include "textx/arpeggio.h"

namespace textx {

    Model::Model(const textx::arpeggio::Match& parsetree, std::shared_ptr<textx::Metamodel> mm) : weak_mm{mm} {
        root = create_model(parsetree, *mm);
    } 

    textx::object::Value create_model(const textx::arpeggio::Match &m, textx::Metamodel& mm) {
        if (m.name.has_value() && m.name.value().starts_with("rule://")) {
            std::string rule_name = m.name.value().substr(7);
            auto &rule = mm[rule_name];
            return {};
        }
        else {
            textx::arpeggio::raise(m.start(), "unexpected, no rule result found here...");
        }
    }

}