#pragma once

#include "textx/assert.h"
#include <string>
#include <variant>
#include <vector>
#include <memory>
#include <unordered_map>

namespace textx {
    class Model;
}

namespace textx {
    class Metamodel;
}

namespace textx::object {
    class Object;
    struct AttributeValue;

    struct ObjectRef {
        std::weak_ptr<textx::Model> tx_model;
        std::string name;
        std::weak_ptr<Object> obj = {};
    };
    struct Value {
        std::variant<std::string, std::shared_ptr<Object>, ObjectRef> data;
        Value(std::string x) : data{x} {}
        Value(std::shared_ptr<Object> x) : data{x} {}
        Value(ObjectRef x) : data{std::move(x)} {}

        bool has_ref() {
            return std::holds_alternative<ObjectRef>(data);
        }

        ObjectRef& ref() {
            TEXTX_ASSERT(std::holds_alternative<ObjectRef>(data), "no ref!");
            return std::get<ObjectRef>(data);
        }

        std::shared_ptr<Object> obj() {
            if (std::holds_alternative<std::shared_ptr<Object>>(data)) {
                return std::get<std::shared_ptr<Object>>(data);
            }
            else {
                TEXTX_ASSERT(std::holds_alternative<ObjectRef>(data));
                return std::get<ObjectRef>(data).obj.lock();
            }
        }

        std::shared_ptr<const Object> obj() const {
            if (std::holds_alternative<std::shared_ptr<Object>>(data)) {
                return std::get<std::shared_ptr<Object>>(data);
            }
            else {
                TEXTX_ASSERT(std::holds_alternative<ObjectRef>(data));
                return std::get<ObjectRef>(data).obj.lock();
            }
        }

        std::string& str() {
            TEXTX_ASSERT(std::holds_alternative<std::string>(data));
            return std::get<std::string>(data);
        }

        const std::string& str() const {
            TEXTX_ASSERT(std::holds_alternative<std::string>(data));
            return std::get<std::string>(data);
        }

        long double f() {
            return std::stold(str());
        }

        long long i() {
            return std::stoll(str(), nullptr, 0);
        }

        unsigned long long u() {
            return std::stoull(str(), nullptr, 0);
        }

        const AttributeValue& operator[](std::string name) const;
        AttributeValue& operator[](std::string name);
    };
    struct AttributeValue {
        std::variant<Value, std::vector<Value>> data = std::vector<Value>{};

        void append(Value v) {
            TEXTX_ASSERT(std::holds_alternative<std::vector<Value>>(data));
            std::get<std::vector<Value>>(data).push_back(v);
        }

        bool has_ref() {
            if (!std::holds_alternative<Value>(data)) {
                return false;
            }
            auto &value = std::get<Value>(data);
            return std::holds_alternative<ObjectRef>(value.data);
        }

        ObjectRef& ref() {
            TEXTX_ASSERT(std::holds_alternative<Value>(data));
            auto &value = std::get<Value>(data);
            TEXTX_ASSERT(std::holds_alternative<ObjectRef>(value.data), "no ref");
            return std::get<ObjectRef>(value.data);
        }

        std::shared_ptr<Object> obj() {
            TEXTX_ASSERT(std::holds_alternative<Value>(data));
            auto &value = std::get<Value>(data);
            if (std::holds_alternative<std::shared_ptr<Object>>(value.data)) {
                return std::get<std::shared_ptr<Object>>(value.data);
            }
            else {
                TEXTX_ASSERT(std::holds_alternative<ObjectRef>(value.data));
                return std::get<ObjectRef>(value.data).obj.lock();
            }
        }

        std::shared_ptr<const Object> obj() const {
            TEXTX_ASSERT(std::holds_alternative<Value>(data));
            auto &value = std::get<Value>(data);
            if (std::holds_alternative<std::shared_ptr<Object>>(value.data)) {
                return std::get<std::shared_ptr<Object>>(value.data);
            }
            else {
                TEXTX_ASSERT(std::holds_alternative<ObjectRef>(value.data));
                return std::get<ObjectRef>(value.data).obj.lock();
            }
        }

        std::string& str() {
            TEXTX_ASSERT(std::holds_alternative<Value>(data));
            auto &value = std::get<Value>(data);
            TEXTX_ASSERT(std::holds_alternative<std::string>(value.data));
            return std::get<std::string>(value.data);
        }

        const std::string& str() const {
            TEXTX_ASSERT(std::holds_alternative<Value>(data));
            auto &value = std::get<Value>(data);
            TEXTX_ASSERT(std::holds_alternative<std::string>(value.data));
            return std::get<std::string>(value.data);
        }

        long double f() {
            return std::stold(str());
        }

        long long i() {
            return std::stoll(str(), nullptr, 0);
        }

        unsigned long long u() {
            return std::stoull(str(), nullptr, 0);
        }

        const AttributeValue& operator[](std::string name) const;
        AttributeValue& operator[](std::string name);
        const Value& operator[](size_t idx) const;
        Value& operator[](size_t idx);
        size_t size() const;
    };

    struct Object {
        std::string type;
        std::weak_ptr<textx::Model> tx_model;
        std::unordered_map<std::string, AttributeValue> attributes;

        const AttributeValue& operator[](std::string name) const;
        AttributeValue& operator[](std::string name);
        void create_attribute_if_not_present(std::string name);
    };

}