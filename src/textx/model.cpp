#include "textx/model.h"
#include "textx/metamodel.h"
#include "textx/arpeggio.h"

namespace textx {

    void Model::init(const std::string_view text, const textx::arpeggio::Match &parsetree, std::shared_ptr<Metamodel> mm) {
       weak_mm = mm;
       root = create_model(text, parsetree, *mm);
    } 

    textx::object::Value Model::create_model(const std::string_view text, const textx::arpeggio::Match &m, textx::Metamodel &mm) {
        if (m.name_starts_with("rule://")) {
            std::string rule_name = m.name.value().substr(7);
            auto &rule = mm[rule_name];
            if (rule.type() == RuleType::match) {
                //std::cout << "match -*- " << m << "\n";
                return std::string(textx::arpeggio::get_str(text, m));
            }
            if (rule.type() == RuleType::common) {
                //std::cout << "common -*- " << m << "\n";
                return create_model_from_common_rule(rule_name, text, m, mm);
            }
            else {
                textx::arpeggio::raise(m.start(), "TODO");
            }
        }
        else {
            textx::arpeggio::raise(m.start(), "unexpected, no rule result found here...\n", m);
        }
    }

    textx::object::Value Model::create_model_from_common_rule(const std::string& rule_name, const std::string_view text, const textx::arpeggio::Match &m0, textx::Metamodel &mm) {
        auto obj = std::make_shared<textx::object::Object>();
        obj->type = rule_name;
        obj->tx_model = shared_from_this(); // store weak ptr

        // traverse tree and stop on "rule://" names..."
        std::function<void(const textx::arpeggio::Match&, bool)> traverse;
        traverse = [&, this](const textx::arpeggio::Match& m, bool first=true) {
            if (m.name_starts_with("rule://") && !first) {
                return;
            }
            else if (m.name_starts_with("assignment://")) {
                std::string attr_name = m.name.value().substr(13);
                //std::cout << "*************** adding attr " << m.name.value() << "\n";
                auto& val = m.children[0];
                obj->create_attribute_if_not_present(attr_name);
                if (mm[rule_name][attr_name].cardinality==AttributeCardinality::scalar) {
                    (*obj)[attr_name].data = create_model(text, val, mm);
                }
                else {
                    (*obj)[attr_name].append(create_model(text, val, mm));
                }
            }
            else {
                for (auto &c: m.children) {
                    traverse(c, false);
                }
            }
        };
        traverse(m0,true);

        //std::cout << m0 << "\n";
        return obj;         
    }

}