#pragma once
#include "textx/model.h"
#include "textx/object.h"
#include <tuple>
#include <variant>
#include <memory>
#include <vector>

namespace textx::scoping {
    using MatchedPath = textx::object::MatchedPath;

    struct Postponed {};

    struct RefResolver {
        /** looks for obj_name in attr_name, starting form origin */
        virtual std::tuple<std::shared_ptr<textx::object::Object>, MatchedPath> resolve(std::shared_ptr<textx::object::Object> origin, std::string obj_name, std::optional<std::string> target_type) const =0;
        virtual ~RefResolver() = default;
    };

    struct PlainNameRefResolver : RefResolver {
        std::tuple<std::shared_ptr<textx::object::Object>, MatchedPath> resolve(std::shared_ptr<textx::object::Object> origin, std::string obj_name, std::optional<std::string> target_type) const override;
    };

    struct FQNRefResolver : RefResolver {
        std::tuple<std::shared_ptr<textx::object::Object>, MatchedPath> resolve(std::shared_ptr<textx::object::Object> origin, std::string obj_name, std::optional<std::string> target_type) const override;
    };

    using PostponedOrObject = std::variant<Postponed, std::shared_ptr<textx::object::Object>>;

    std::vector<std::string> separate_name(std::string obj_name);
    std::shared_ptr<textx::object::Object> dot_separated_name_search(std::shared_ptr<textx::object::Object> origin, const std::vector<std::string> &v_obj_name, std::optional<std::string> target_type, size_t idx=0);
    inline std::shared_ptr<textx::object::Object> dot_separated_name_search(std::shared_ptr<textx::object::Object> origin, std::string obj_name, std::optional<std::string> target_type=std::nullopt) {
        auto v = separate_name(obj_name);
        return dot_separated_name_search(origin, v, target_type);
    }
    inline bool is_valid(PostponedOrObject x) {
        return std::holds_alternative<std::shared_ptr<textx::object::Object>>(x) && std::get<1>(x)!=nullptr;
    }
    inline bool is_postponed(PostponedOrObject x) {
        return std::holds_alternative<Postponed>(x);
    }
    inline bool is_valid_or_postponed(PostponedOrObject x) {
        return is_postponed(x) || is_valid(x);
    }
}
