#pragma once
#include "textx/model.h"
#include "textx/object.h"

namespace textx::scoping {
    struct RefResolver {
        /** looks for obj_name in attr_name, starting form origin */
        virtual std::shared_ptr<textx::object::Object> resolve(std::shared_ptr<textx::object::Object> origin, std::string attr_name, std::string obj_name) const =0;
        virtual ~RefResolver() = default;
    };

    struct PlainNameRefResolver : RefResolver {
        std::shared_ptr<textx::object::Object> resolve(std::shared_ptr<textx::object::Object> origin, std::string attr_name, std::string obj_name) const override;
    };

    struct FQNRefResolver : RefResolver {
        std::shared_ptr<textx::object::Object> resolve(std::shared_ptr<textx::object::Object> origin, std::string attr_name, std::string obj_name) const override;
    };
}
