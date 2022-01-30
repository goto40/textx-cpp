#include "textx/export.h"
#include "textx/assert.h"
#include "textx/metamodel.h"
#include <fstream>
#include <stdexcept>
#include <filesystem>

namespace {
    namespace intern {
        std::filesystem::path rel_path_to_json_for_second(std::filesystem::path original_model, std::filesystem::path second) {
            if (original_model==second) {
                auto ret = std::filesystem::path{std::string{original_model.stem()}+".json"};
                return ret;
            }
            else {
                auto rel = std::filesystem::relative(second, original_model);
                if (std::string{rel}.size()==0) {
                    rel = second;
                }
                auto ret = rel.parent_path() / ((std::string{rel.stem()}+".json"));
                return ret;
            }
        }
        std::string path_to_obj(std::shared_ptr<const textx::object::Object> obj) {
            if (obj->parent()==nullptr) return "";
            else {
                auto p = obj->parent();
                for (auto  [n,a]: p->attributes) {
                    if (a.is_pure_obj() && a.obj()==obj) {
                        return path_to_obj(p)+"/"+n;
                    }
                    else if (a.is_list()) {
                        size_t idx=0;
                        for (auto e: a) {
                            if (e.is_obj() && e.obj()==obj) {
                                return path_to_obj(p)+"/"+n+"["+std::to_string(idx)+"]";
                            }
                            idx++;
                        }
                    }
                }
            }
            throw std::runtime_error("unexpected navigation through object error.");
        }
        void save(std::ostream &o, std::shared_ptr<const textx::object::Object> obj, size_t indent=0, std::string schema_url="");
        template<class T>
        void save_attr(std::ostream &o, std::shared_ptr<const textx::object::Object> obj, const std::string &name, const T& attr, size_t indent) {
            if (attr.is_null()) {
                o << "{}";
            }
            else if (attr.is_boolean()) {
                o << (attr.boolean()?"true":"false");
            }
            else if (attr.is_str()) {
                o << "\"" << attr.str() << "\"";
            }
            else if (attr.is_pure_obj()) {
                save(o, attr.obj(), indent+1);
            }
            else if (attr.is_ref()) {
                if (attr.obj()->tx_model() == obj->tx_model()) {
                    // within file:
                    o << "{\"$ref\": \"" << "#" << path_to_obj(attr.obj()) << "\"}";
                }
                else {
                    // cross file
                    auto f0 = obj->tx_model()->tx_filename();
                    auto f1 = attr.obj()->tx_model()->tx_filename();
                    auto fn = rel_path_to_json_for_second(f0,f1);
                    // note: std::string{fn} is required in order not to enclose the file in '"'. 
                    o << "{\"$ref\": \"" << std::string{fn} << "#" << path_to_obj(attr.obj()) << "\"}";
                }
            }
            else {
                TEXTX_ASSERT(false, "unexpected case for attr ", name, " in obj of type ", obj->type);
            }
        }
        void save(std::ostream &o, std::shared_ptr<const textx::object::Object> obj, size_t indent, std::string schema_url) {
            auto type = obj->type;
            o << "{";
            if (!schema_url.empty()) {
                o << "\n" << std::string((indent+1)*2, ' ') << "\"" << "$schema" << "\":" << "\"" << schema_url << "\",";
            }
            o << "\n" << std::string((indent+1)*2, ' ') << "\"" << "$type" << "\":" << "\"" << type << "\"";
            bool first=false;
            for (auto& [name, attr]: obj->attributes) {
                if (!first) o << ",";
                o << "\n";
                first = false;
                o << std::string((indent+1)*2, ' ') << "\"" << name << "\":";
                if (attr.is_list()) {
                    o << "[";
                    bool ifirst=true;
                    for (auto &e: attr) {
                        if (!ifirst) o << ",";
                        o << "\n";
                        ifirst = false;
                        o << std::string((indent+2)*2, ' ');
                        save_attr(o, obj, name, e, indent+1);
                    }
                    o << "\n";
                    o << std::string((indent+1)*2, ' ') << "]";
                }
                else {
                    save_attr(o, obj, name, attr, indent);
                }
            }
            o << "\n";
            o << std::string(indent*2, ' ') << "}";
        }

        std::string get_schema_url_filename(std::shared_ptr<textx::Metamodel> mm) {
            return mm->tx_grammar_name()+"-schema.json";
        }

    }    
}

namespace textx {

    void save_model_as_json(std::shared_ptr<textx::Model> model, bool save_all, std::string schema_url, std::string dest_str) {
        if (!std::filesystem::exists(dest_str)) {
            std::filesystem::create_directory(dest_str);
        }

        if (schema_url.empty()) {
            schema_url = "./"+intern::get_schema_url_filename(model->tx_metamodel());
            save_metamodel_as_json_schema(model->tx_metamodel(),true,"./",dest_str);
        }

        TEXTX_ASSERT(model->tx_filename().size()>0);
        auto source0 = std::filesystem::path{model->tx_filename()};

        std::unordered_set<std::shared_ptr<textx::Model>> all_models={};
        if (save_all) {
            all_models = {model->get_all_referenced_models()};
        }
        else {
            all_models.insert(model);
        }
        for(auto &m: all_models) {
            TEXTX_ASSERT(m->tx_filename().size()>0);
            auto source = std::filesystem::path{m->tx_filename()};
            auto fn = std::filesystem::path(dest_str) / intern::rel_path_to_json_for_second(source0, source);
            //std::cout << "CREATING " << fn << "\n";
            std::ofstream f{fn};
            save_model_as_json(m, f, schema_url);            
        }
    }

    void save_model_as_json(std::shared_ptr<textx::Model> model, std::ostream &o, std::string schema_url) {
        //TODO decide if file has to be generated or not, based on timestamp of model and dest (use exe date for internal models)
        TEXTX_ASSERT(model->val().is_obj());
        intern::save(o, model->val().obj(), 0, schema_url);
        o << "\n";
    }

    void save_metamodel_as_json_schema(std::shared_ptr<textx::Metamodel> mm, bool save_all, std::string url_prefix, std::string dest_str) {
        auto fn = std::filesystem::path(dest_str) / intern::get_schema_url_filename(mm);
        std::ofstream f{fn};
        save_metamodel_as_json_schema(mm, f, url_prefix);
        // TODO save_all -- save all imported mm as well
    }

    void save_metamodel_as_json_schema(std::shared_ptr<textx::Metamodel> mm, std::ostream &s, std::string url_prefix) {
        s << "{" << "\n";
        s << "  \"$schema\": \"http://json-schema.org/draft-07/schema\",\n";
        s << "  \"$ref\": \"#/$def/" << mm->tx_main_rule_name() << "\",\n";
        s << "  \"$def\": {\n";
        s << "    \"$ref\": { \"type\": \"object\", \"properties\": { \"$ref\": \"string\" }},\n";
        bool first = true;
        for (auto& r: *mm) {
            if (r.second.type() == textx::RuleType::common) {
                if (!first) s << ",\n";
                s << "    \"" << r.first << "\": {\n";
                s << "      \"type\": \"object\",\n";
                s << "      \"additionalProperties\": false,\n";
                s << "      \"properties\": {\n";
                s << "        \"$schema\": {},\n";
                s << "        \"$type\": \"string\",\n";
                // attributes can be scalar, list, boolean
                // scalar/list can have a type or be a string
                size_t n = r.second.get_attribute_info().size();
                size_t idx=0;
                for (auto &[attr_name, attr]: r.second.get_attribute_info()) {
                    std::string attr_name_json = attr_name; // std::string("^") + attr_name + "$";
                    s << "        \"" << attr_name_json << "\": {\n";
                    if (attr.is_boolean()) { // TODO use maybe_boolean and also handle maybe_str etc. here somewhere...
                        s << "          " << "\"type\": \"boolean\"\n";
                    }
                    else {
                        std::string extra_intend="";
                        if (attr.cardinality==AttributeCardinality::list) {
                            s << "          " << "\"type\": \"array\", \"items\": {\n";
                            extra_intend="  ";
                        }
                        else {
                            TEXTX_ASSERT(attr.cardinality==AttributeCardinality::scalar);
                        }

                        if (!attr.type.has_value()) { // no type --> string
                            s << "          " << extra_intend << "\"type\": \"string\"\n";
                        }
                        else if (mm->operator[](attr.type.value()).type() == RuleType::match ) {
                            s << "          " << extra_intend << "\"type\": \"string\"\n";
                        }
                        else {
                            // TODO also allow references in the model here... (!)
                            s << "          " << extra_intend << "\"oneOf\": [\n";
                            s << "          " << extra_intend << "  {\"$ref\": \"#/$def/"<< attr.type.value() << "\"},\n";
                            s << "          " << extra_intend << "  {\"$ref\": \"#/$def/$ref\"}\n";
                            s << "          " << extra_intend << "]\n";
                        }

                        if (attr.cardinality==AttributeCardinality::list) {
                            s << "          " << "}\n";
                        }
                    }
                    s << "        }";
                    if (idx!=n-1) {
                        s << ",";
                    }
                    s << "\n";
                    idx++;
                }
                s << "      }\n";
                s << "    }";
                first = false;
            }
            else if (r.second.type() == textx::RuleType::abstract) {
                if (!first) s << ",\n";
                s << "    \"" << r.first << "\": {\n";
                s << "      " << "\"oneOf\": [ \"string\", \n";
                size_t n = r.second.tx_inh_by().size();
                size_t idx=0;
                for(const auto &rule_name: r.second.tx_inh_by()) {
                    // TODO modify reference if rule is from imported grammar
                    s << "        " << "{ \"$ref\": \"#/$def/"<< rule_name << "\" }";
                    if (idx!=n-1) {
                        s << ",";
                    }
                    s << "\n";
                    idx++;
                }
                s << "      " << "]\n";
                s << "    }";
                first = false;
            }
        }
        s << "\n  }\n";
        s << "}" << "\n";
    }

}