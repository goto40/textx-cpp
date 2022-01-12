#include "textx/export.h"
#include "textx/assert.h"
#include <fstream>
#include <stdexcept>

namespace {
    namespace intern {
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
                o << "{\"ref\": \"" << "TODO" << "\"}";
            }
            else {
                TEXTX_ASSERT(false, "unexpected case for attr ", name, " in obj of type ", obj->type);
            }
        }
        void save(std::ostream &o, std::shared_ptr<const textx::object::Object> obj, size_t indent) {
            o << "{";
            bool first=true;
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
        std::ofstream f{model->tx_filename()};
        save_as_simple_json(model, f);
    }

    void save_as_simple_json(std::shared_ptr<textx::Model> model, std::ostream &o) {
        TEXTX_ASSERT(model->val().is_obj());
        intern::save(o, model->val().obj());
        o << "\n";
    }

}