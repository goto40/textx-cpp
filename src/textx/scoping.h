#pragma once
#include "textx/model.h"
#include "textx/object.h"

namespace textx::scoping {
    struct RefResolver {
        /** looks for obj_name in attr_name, starting form origin */
        virtual std::shared_ptr<textx::object::Object> resolve(std::shared_ptr<textx::object::Object> origin, std::string obj_name) const =0;
        virtual ~RefResolver() = default;
    };

    struct PlainNameRefResolver : RefResolver {
        std::shared_ptr<textx::object::Object> resolve(std::shared_ptr<textx::object::Object> origin, std::string obj_name) const override;
    };

    struct FQNRefResolver : RefResolver {
        std::shared_ptr<textx::object::Object> resolve(std::shared_ptr<textx::object::Object> origin, std::string obj_name) const override;
    };

    std::vector<std::string> separate_name(std::string obj_name);
    std::shared_ptr<textx::object::Object> dot_separated_name_search(std::shared_ptr<textx::object::Object> origin, const std::vector<std::string> &v_obj_name, size_t idx=0);
    inline std::shared_ptr<textx::object::Object> dot_separated_name_search(std::shared_ptr<textx::object::Object> origin, std::string obj_name) {
        auto v = separate_name(obj_name);
        return dot_separated_name_search(origin, v);
    }
}
