#pragma once

#include "textx/assert.h"
#include "textx/arpeggio.h"
#include <string>
#include <variant>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

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
        std::string rule;
        std::string attr;
        std::weak_ptr<Object> parent = {};
        std::weak_ptr<Object> obj = {};
    };
    struct Value {
        std::variant<std::string, std::shared_ptr<Object>, ObjectRef> data;
        textx::arpeggio::TextPosition pos;
        Value(std::string x,textx::arpeggio::TextPosition pos) : data{x},pos{pos} {}
        Value(std::shared_ptr<Object> x,textx::arpeggio::TextPosition pos) : data{x},pos{pos} {}
        Value(ObjectRef x,textx::arpeggio::TextPosition pos) : data{std::move(x)},pos{pos} {}

        bool is_ref() const {
            return std::holds_alternative<ObjectRef>(data);
        }
        bool is_pure_obj() const {
            return std::holds_alternative<std::shared_ptr<Object>>(data);
        }
        bool is_obj() const {
            return is_pure_obj() || is_ref();
        }
        bool is_str() const {
            return std::holds_alternative<std::string>(data);
        }

        ObjectRef& ref() {
            TEXTX_ASSERT(std::holds_alternative<ObjectRef>(data), "no ref!");
            return std::get<ObjectRef>(data);
        }

        const ObjectRef& ref() const {
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

        void print(std::ostream& o, size_t indent=0) const;
        friend std::ostream& operator<<(std::ostream& o, const Value&v) {
            v.print(o);
            return o;
        }
   };
    struct AttributeValue {
        std::variant<Value, std::vector<Value>> data = std::vector<Value>{};

        void append(Value v) {
            TEXTX_ASSERT(std::holds_alternative<std::vector<Value>>(data));
            std::get<std::vector<Value>>(data).push_back(v);
        }

        bool is_ref() const {
            if (!std::holds_alternative<Value>(data)) {
                return false;
            }
            auto &value = std::get<Value>(data);
            return std::holds_alternative<ObjectRef>(value.data);
        }
        bool is_pure_obj() const {
            if (!std::holds_alternative<Value>(data)) {
                return false;
            }
            auto &value = std::get<Value>(data);
            return std::holds_alternative<std::shared_ptr<Object>>(value.data);
        }
        bool is_obj() const {
            return is_pure_obj() || is_ref();
        }
        bool is_str() const {
            if (!std::holds_alternative<Value>(data)) {
                return false;
            }
            auto &value = std::get<Value>(data);
            return std::holds_alternative<std::string>(value.data);
        }


        ObjectRef& ref() {
            TEXTX_ASSERT(std::holds_alternative<Value>(data));
            auto &value = std::get<Value>(data);
            TEXTX_ASSERT(std::holds_alternative<ObjectRef>(value.data), "no ref");
            return std::get<ObjectRef>(value.data);
        }

        const ObjectRef& ref() const {
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
        std::weak_ptr<textx::Model> weak_model;
        std::unordered_map<std::string, AttributeValue> attributes;

        std::shared_ptr<textx::Model> tx_model() { return weak_model.lock(); }

        bool has_attr(std::string n) { return attributes.count(n)>0; }
        const AttributeValue& operator[](std::string name) const;
        AttributeValue& operator[](std::string name);
        void create_attribute_if_not_present(std::string name);

        void print(std::ostream& o, size_t indent=0) const;
    };

    inline void traverse(textx::object::Value& v, std::function<void(textx::object::Value&)> f) {
        f(v);
        if (v.is_str()) {
            // nothing
        }
        else if (v.is_ref()) {
            // nothing
        }
        else if (v.is_pure_obj()) {
            for (auto &[k,av]: v.obj()->attributes) {
                if (std::holds_alternative<textx::object::Value>(av.data)) {
                    traverse(std::get<textx::object::Value>(av.data),f);
                }
                else {
                    for (auto &iv: std::get<std::vector<textx::object::Value>>(av.data)) {
                        traverse(iv,f);
                    }
                }
            }
        }
        else {
            textx::arpeggio::raise(v.pos, "unexpected situation");
        }
    }
}