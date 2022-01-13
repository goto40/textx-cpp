#include "textx/export.h"
#include "textx/assert.h"
#include "textx/metamodel.h"
#include <fstream>
#include <stdexcept>
#include <filesystem>

namespace {
    namespace intern {
        std::filesystem::path rel_path_to_json_for_second(std::filesystem::path original_model, std::filesystem::path second) {
            auto rel = std::filesystem::relative(second, original_model);
            return rel.parent_path() / (std::string{rel.stem()}+".json");
        }
        std::string path_to_obj(std::shared_ptr<const textx::object::Object> obj) {
            //path through model + index if in array
            return "TODO";
        }
        void save(std::ostream &o, std::shared_ptr<const textx::object::Object> obj, size_t indent=0);
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
                    o << "{\"ref\": \"" << "#/" << path_to_obj(attr.obj()) << "\"}";
                }
                else {
                    o << "{\"ref\": \"" << "FILE-TODO" << "#/" << path_to_obj(attr.obj()) << "\"}";
                }
            }
            else {
                TEXTX_ASSERT(false, "unexpected case for attr ", name, " in obj of type ", obj->type);
            }
        }
        void save(std::ostream &o, std::shared_ptr<const textx::object::Object> obj, size_t indent) {
            auto type = obj->type;
            o << "{";
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
    }    
}

namespace textx {

    void save_as_simple_json(std::shared_ptr<textx::Model> model) {
        TEXTX_ASSERT(model->tx_filename().size()>0);
        auto source = model->tx_filename();
        auto fn = std::string{std::filesystem::path{source}.stem()}+".json";
        std::ofstream f{fn};
        save_as_simple_json(model, f);
    }

    void save_as_simple_json(std::shared_ptr<textx::Model> model, std::ostream &o) {
        TEXTX_ASSERT(model->val().is_obj());
        intern::save(o, model->val().obj());
        o << "\n";
    }

}