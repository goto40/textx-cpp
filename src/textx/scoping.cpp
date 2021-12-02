#include "textx/scoping.h"

namespace textx::scoping {
    std::shared_ptr<textx::object::Object> DefaultRefResolver::resolve(std::shared_ptr<textx::object::Object> origin, std::string attr_name, std::string obj_name) {
        auto m = origin->tx_model.lock();

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
                    //std::cout << "FOUND!\n";
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
        return traverse(m->val());
    }
}