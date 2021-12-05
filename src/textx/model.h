#pragma once
#include "textx/object.h"
#include "textx/textx_grammar_parsetree.h"
#include <memory>

namespace textx {

    class Model : public std::enable_shared_from_this<Model> {
        std::weak_ptr<Metamodel> weak_mm;
        textx::object::Value root={std::shared_ptr<textx::object::Object>{},{}}; // nullptr
        textx::object::Value create_model(const std::string_view text, const textx::arpeggio::Match &m, textx::Metamodel &mm, std::shared_ptr<textx::object::Object> parent);
        textx::object::Value create_model_from_common_rule(const std::string& rule_name, const std::string_view text, const textx::arpeggio::Match &m0, textx::Metamodel &mm, std::shared_ptr<textx::object::Object> parent);
        textx::object::Value create_model_from_abstract_rule(const std::string& rule_name, const std::string_view text, const textx::arpeggio::Match &m0, textx::Metamodel &mm, std::shared_ptr<textx::object::Object> parent);
        Model() = default;
        void init(const std::string_view text, const textx::arpeggio::Match &parsetree, std::shared_ptr<Metamodel> mm);
        
        /** return the number of unresolved refs */
        size_t resolve_references();
        friend textx::Metamodel;
    public:
        std::shared_ptr<textx::Metamodel> tx_metamodel() { return weak_mm.lock(); }
        textx::object::Value& val() { 
            return root;
        }
        const textx::object::Value& val() const {
            return root;
        }

        // convenience functions:
        bool is_ref() const { return val().is_ref(); }
        bool is_pure_obj() const { return val().is_pure_obj(); }
        bool is_obj() const { return val().is_obj(); }
        bool is_str() const { return val().is_str(); }
        textx::object::ObjectRef& ref() { return val().ref(); }
        std::shared_ptr<textx::object::Object> obj() { return val().obj(); }
        std::shared_ptr<const textx::object::Object> obj() const { return val().obj(); }
        std::string str() const { return val().str(); }
        long double f() const { return val().f(); }
        long long i() const { return val().i(); }
        unsigned long long u() const { return val().u(); }
        const textx::object::AttributeValue& operator[](std::string name) const { return val()[name]; }
        textx::object::AttributeValue& operator[](std::string name) { return val()[name]; }
        std::shared_ptr<textx::object::Object> fqn(std::string name);
    };
}