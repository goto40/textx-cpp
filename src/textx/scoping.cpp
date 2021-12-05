#include "textx/scoping.h"
#include "textx/metamodel.h"
#include <sstream>
#include <iostream>

namespace textx::scoping {

    std::shared_ptr<textx::object::Object> PlainNameRefResolver::resolve(std::shared_ptr<textx::object::Object> origin, std::string obj_name) const {
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
            else if (v.is_pure_obj()) {
                // std::cout << "resolve ... " << v.obj()->type << "\n";
                // if (v.obj()->has_attr("name")) {
                //     std::cout << "name ='" <<(*v.obj())["name"].str()<< "' ==? '" << obj_name<<"'\n";
                // }
                if (v.obj()->has_attr("name") && (*v.obj())["name"].str()==obj_name) {
                    //std::cout << "FOUND!" << obj_name << "\n";
                    return v.obj();
                } 
                for (auto &[k,av]: v.obj()->attributes) {
                    if (std::holds_alternative<textx::object::Value>(av.data)) {
                        auto p = traverse(std::get<textx::object::Value>(av.data));
                        if (p) return p;
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
                textx::arpeggio::raise(v.pos, "unexpected situaltion");
            }

            return nullptr;
        };
        // own model:
        {
            auto p = traverse(m->val());
            if (p) return p;
        }
        // build in models:
        for (auto im: mm->tx_builtin_models()) {
            auto p = traverse(im->val());
            if (p) return p;
        }
        return nullptr;
    }

    std::shared_ptr<textx::object::Object> FQNRefResolver::resolve(std::shared_ptr<textx::object::Object> origin, std::string obj_name) const {
        auto m = origin->tx_model();
        auto mm = m->tx_metamodel();

        auto v_obj_name = separate_name(obj_name);
        //auto res = search(origin, v);
        return nullptr;
    }

    std::vector<std::string> separate_name(std::string obj_name) {
        std::istringstream f_obj_name{obj_name};
        std::vector<std::string> v_obj_name = {};
        {
            std::string x;
            while(std::getline(f_obj_name, x, '.')) {
                v_obj_name.push_back(x);
            }
        }
        return v_obj_name;
    }
    std::shared_ptr<textx::object::Object> dot_separated_name_search(std::shared_ptr<textx::object::Object> origin, std::vector<std::string> v_obj_name, size_t idx) {
        if (idx==v_obj_name.size()) {
            return origin;
        }
        TEXTX_ASSERT(idx<v_obj_name.size());
 
        auto check_obj_and_name=[&](auto &attr) {
            return attr.is_pure_obj() && attr.obj()->has_attr("name") && attr["name"].is_str() && attr["name"].str()==v_obj_name[idx];
        };

        for(auto& [aname,attr]: origin->attributes) {
            if (check_obj_and_name(attr)) {
                auto res = dot_separated_name_search(attr.obj(),v_obj_name,idx+1);
                if (res) return res;
            }
            if (attr.is_list()) {
                for(size_t i=0;i<0;i++) {
                    if (check_obj_and_name(attr[i])) {
                        auto res = dot_separated_name_search(attr[i].obj(),v_obj_name,idx+1);
                        if (res) return res;
                    }
                }
            }
        }
        return nullptr;
    }

}