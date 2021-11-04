#include "textx/scoping.h"
#include "textx/metamodel.h"
#include "textx/arpeggio.h"
#include <sstream>
#include <iostream>

namespace textx::scoping {

    std::tuple<std::shared_ptr<textx::object::Object>, MatchedPath> PlainNameRefResolver::resolve(std::shared_ptr<textx::object::Object> origin, std::string obj_name, std::optional<std::string> target_type) const {
        auto m = origin->tx_model();
        auto mm = m->tx_metamodel();

        std::function<std::shared_ptr<textx::object::Object>(textx::object::Value&)> traverse;
        traverse = [&, this](textx::object::Value& v) -> std::shared_ptr<textx::object::Object> {
            if (v.is_str()) {
                // nothing
            }
            else if (v.is_ref()) {
                // nothing
            }
            else if (v.is_boolean()) {
                // nothing
            }
            else if (v.is_pure_obj()) {
                // std::cout << "resolve ... " << v.obj()->type << "\n";
                // if (v.obj()->has_attr("name")) {
                //     std::cout << "name ='" <<(*v.obj())["name"].str()<< "' ==? '" << obj_name<<"'\n";
                // }
                if (!v.is_null() && v.obj()->has_attr("name") && (*v.obj())["name"].str()==obj_name) {
                    if(target_type.has_value()) {
                        //use master mm! 
                        // no: auto &mm = *v.obj()->tx_model()->tx_metamodel();
                        if (!mm->is_base_of(target_type.value(),v.obj()->type)) {
                            textx::arpeggio::raise(v.obj()->pos,"'", obj_name, "' has not expected type '", target_type.value(), "'");
                        }
                    }
                    //std::cout << "FOUND!" << obj_name << "\n";
                    return v.obj();
                } 
                for (auto &[k,av]: v.obj()->attributes) {
                    if (std::holds_alternative<textx::object::Value>(av.data)) {
                        auto &the_value = std::get<textx::object::Value>(av.data);
                        if (!the_value.is_null()) {
                            auto p = traverse(the_value);
                            if (p) return p;
                        }
                    }
                    else {
                        for (auto &iv: std::get<std::vector<textx::object::Value>>(av.data)) {
                            auto p = traverse(iv);
                            if (p) return p;
                        }
                    }
                }
            }
            else {
                textx::arpeggio::raise(v.pos, "unexpected situaltion (scoping)");
            }

            return nullptr;
        };
        // own model:
        {
            auto p = traverse(m->val());
            if (p) return {p, {}};
        }
        // imported/builtin models:
        for (auto im: m->tx_imported_models()) {
            auto p = traverse(im.lock()->val());
            if (p) return {p, {}};
        }
        return {nullptr, {}};
    }

    std::tuple<std::shared_ptr<textx::object::Object>, MatchedPath> FQNRefResolver::resolve(std::shared_ptr<textx::object::Object> origin, std::string obj_name, std::optional<std::string> target_type) const {
        auto m = origin->tx_model();
        auto mm = m->tx_metamodel();
        auto v_obj_name = separate_name(obj_name);

        // own model
        while(origin!=nullptr) {
            auto res = dot_separated_name_search(origin, v_obj_name, target_type);
            if (res) return {res, {}};
            origin = origin->parent();
        }
        // imported/builtin models:
        for (auto wim: m->tx_imported_models()) {
            auto im = wim.lock();
            if (im->val().is_obj()) {
                origin = im->val().obj();
                //std::cout << origin << "\n";
                auto res = dot_separated_name_search(origin, v_obj_name, target_type);
                if (res) return {res, {}};
            }
        }
        return {nullptr, {}};
    }

    std::vector<std::string> separate_name(std::string obj_name) {
        std::istringstream f_obj_name{obj_name};
        TEXTX_ASSERT(obj_name.size()>0, "empty names are not allowed");
        TEXTX_ASSERT(obj_name[0]!='.', obj_name, "names mut not start with a dot");
        TEXTX_ASSERT(obj_name[obj_name.size()-1]!='.', obj_name, "names mut not start with a dot");
        std::vector<std::string> v_obj_name = {};
        {
            std::string x;
            while(std::getline(f_obj_name, x, '.')) {
                TEXTX_ASSERT(x.size()>0, obj_name, "all parts must be non empty strings");
                v_obj_name.push_back(x);
            }
        }
        return v_obj_name;
    }
    std::shared_ptr<textx::object::Object> dot_separated_name_search(std::shared_ptr<textx::object::Object> origin, const std::vector<std::string> &v_obj_name, std::optional<std::string> target_type, size_t idx) {
        if (idx==v_obj_name.size()) {
            if(target_type.has_value()) {
                auto &mm = *origin->tx_model()->tx_metamodel();
                if (!mm.is_base_of(target_type.value(),origin->type)) {
                    textx::arpeggio::raise(origin->pos,"'", v_obj_name[idx-1], "' has not expected type '", target_type.value(), "'");
                }
            }
            return origin;
        }
        TEXTX_ASSERT(idx<v_obj_name.size());
 
        auto check_obj_and_name=[&](auto &attr) {
            return attr.is_pure_obj() && attr.obj()->has_attr("name") && attr["name"].is_str() && attr["name"].str()==v_obj_name[idx];
        };

        for(auto& [aname,attr]: origin->attributes) {
            if (attr.is_null()) continue; // optional attributes (x=ID)?
            if (check_obj_and_name(attr)) {
                auto res = dot_separated_name_search(attr.obj(), v_obj_name, target_type, idx+1);
                if (res) return res;
            }
            else if (attr.is_list()) {
                for(size_t i=0;i<attr.size();i++) {
                    if (check_obj_and_name(attr[i])) {
                        auto res = dot_separated_name_search(attr[i].obj(), v_obj_name, target_type, idx+1);
                        if (res) return res;
                    }
                }
            }
        }
        return nullptr;
    }
}