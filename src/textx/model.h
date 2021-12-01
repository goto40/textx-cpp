#pragma once
#include "textx/object.h"
#include "textx/textx_grammar_parsetree.h"
#include <memory>

namespace textx {

    class Model : public std::enable_shared_from_this<Model> {
        std::weak_ptr<Metamodel> weak_mm;
        textx::object::Value root={std::shared_ptr<textx::object::Object>{},{}}; // nullptr
        textx::object::Value create_model(const std::string_view text, const textx::arpeggio::Match &m, textx::Metamodel &mm);
        textx::object::Value create_model_from_common_rule(const std::string& rule_name, const std::string_view text, const textx::arpeggio::Match &m0, textx::Metamodel &mm);
        textx::object::Value create_model_from_abstract_rule(const std::string& rule_name, const std::string_view text, const textx::arpeggio::Match &m0, textx::Metamodel &mm);
        Model() = default;
        void init(const std::string_view text, const textx::arpeggio::Match &parsetree, std::shared_ptr<Metamodel> mm);
        
        /** return the number of unresolved refs */
        size_t resolve_references();
        friend textx::Metamodel;
    public:
        textx::object::Value& val() { 
            return root;
        }
        const textx::object::Value& val() const {
            return root;
        }
    };
}