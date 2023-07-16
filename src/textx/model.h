#pragma once
#include "textx/arpeggio.h"
#include "textx/object.h"
#include "textx/rule.h"
#include <memory>

namespace textx {

    class Model : public std::enable_shared_from_this<Model> {
        std::weak_ptr<Metamodel> weak_mm;
        textx::object::Value root={std::shared_ptr<textx::object::Object>{},{}}; // nullptr
        textx::object::Value create_model(const std::string_view text, std::shared_ptr<const textx::arpeggio::Match> m, textx::Metamodel &mm, std::shared_ptr<textx::object::Object> parent);
        textx::object::Value create_model_from_common_rule(const std::string& rule_name, const std::string_view text, std::shared_ptr<const textx::arpeggio::Match> m0, textx::Metamodel &mm, std::shared_ptr<textx::object::Object> parent);
        textx::object::Value create_model_from_abstract_rule(const std::string& rule_name, const std::string_view text, std::shared_ptr<const textx::arpeggio::Match> m0, textx::Metamodel &mm, std::shared_ptr<textx::object::Object> parent);
        Model() = default;
        void init(const std::string_view filename, const std::string_view text, std::shared_ptr<const textx::arpeggio::Match> parsetree, std::shared_ptr<Metamodel> mm);
        std::vector<std::weak_ptr<textx::Model>> weak_imported_models;
        std::string model_text={};
        std::string model_filename={};
        arpeggio::ParserResult parsetree={};
        arpeggio::ParserState parsestate={};
        std::unordered_map<size_t, std::vector<arpeggio::CompletionInfo>> m_completionInfo;
        std::vector<arpeggio::TextxErrorEntry> m_errors;
        
        /** return the number of unresolved refs */
        size_t resolve_references();
        friend textx::Metamodel;
    public:
        bool ok() { return m_errors.size()==0; }
        void add_error(arpeggio::TextxErrorEntry e) { m_errors.push_back(e); }
        void set_filename_info(std::string f) { model_filename=f; }
        std::shared_ptr<textx::Metamodel> tx_metamodel() const { return weak_mm.lock(); }
        textx::object::Value& val() { 
            return root;
        }
        const auto& tx_imported_models() const { return weak_imported_models; }
        void add_imported_model(std::shared_ptr<textx::Model> im) { 
            if (std::find_if(weak_imported_models.begin(), weak_imported_models.end(), [&](auto &x){
                return x.lock() == im;
            })==weak_imported_models.end()) {
                weak_imported_models.push_back(im);
            }
        }

        const std::string& tx_text() { return model_text; };
        const std::string& tx_filename() { return model_filename; };
       
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
        textx::object::AttributeValue fqn_attributes(std::string name) { return val().obj()->fqn_attributes(name); }

        std::unordered_set<std::shared_ptr<textx::Model>> get_all_referenced_models();
    };
}