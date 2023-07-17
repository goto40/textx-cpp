#pragma once

#include "textx/assert.h"
#include "textx/arpeggio.h"
#include <string>
#include <variant>
#include <utility>
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

    using MatchedPath = std::vector<std::weak_ptr<Object>>;

    struct ObjectRef {
        std::weak_ptr<textx::Model> tx_model;
        std::string name;
        std::string rule;
        std::string target_type;
        std::string attr;
        std::weak_ptr<Object> parent = {};
        std::weak_ptr<Object> obj = {};
        MatchedPath objpath = {};
    };

    using MatchText = std::pair<std::string, std::string>; /// first: text, second: rule-type
    struct Value {
        std::variant<MatchText, std::shared_ptr<Object>, ObjectRef, bool> data;
        arpeggio::TextPosition pos;
        std::shared_ptr<const textx::arpeggio::Match> match; // from here, you get the text position 
        Value(MatchText x,std::shared_ptr<const textx::arpeggio::Match> m) : data{x},match{m},pos(m->start()) {}
        Value(std::shared_ptr<Object> x,std::shared_ptr<const textx::arpeggio::Match> m) : data{x},match{m},pos((m!=nullptr)?m->start():0) {}
        Value(ObjectRef x,std::shared_ptr<const textx::arpeggio::Match> m) : data{std::move(x)},match{m},pos(m->start()) {}
        Value(bool x,std::shared_ptr<const textx::arpeggio::Match> m) : data{x},match{m},pos(m->start()) {}

        bool is_boolean() const {
            return std::holds_alternative<bool>(data);
        }

        bool is_ref() const {
            return std::holds_alternative<ObjectRef>(data);
        }

        bool is_null() const {
            if (is_obj() && obj()==nullptr) {
                return true;
            }
            else {
                return false;
            }
        }
        bool is_resolved() const {
            TEXTX_ASSERT(is_ref());
            return obj()!=nullptr;
        }        
        bool is_pure_obj() const {
            return std::holds_alternative<std::shared_ptr<Object>>(data);
        }
        bool is_obj() const {
            return is_pure_obj() || is_ref();
        }
        bool is_str() const {
            return std::holds_alternative<MatchText>(data);
        }
        bool is_number() const {
            return std::holds_alternative<MatchText>(data) && 
            (std::get<MatchText>(data).second=="NUMBER" || std::get<MatchText>(data).second=="INT" || std::get<MatchText>(data).second=="FLOAT" || std::get<MatchText>(data).second=="STRICTFLOAT");
        }

        bool boolean() const {
            TEXTX_ASSERT(std::holds_alternative<bool>(data), "no boolean");
            return std::get<bool>(data);
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
                auto res = std::get<ObjectRef>(data).obj.lock();
                TEXTX_ASSERT(res!=nullptr, "unexpected: reference expired/illegal");
                return res;
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

        std::string str() const {
            TEXTX_ASSERT(std::holds_alternative<MatchText>(data), " at ", this->pos);
            if (std::get<MatchText>(data).second=="STRING") {
                // TODO handle \n, \" etc...
                return std::get<MatchText>(data).first.substr(1,std::get<MatchText>(data).first.size()-2);
            }
            else {
                return std::get<MatchText>(data).first;
            }
        }

        long double f() const {
            return std::stold(str());
        }

        long long i() const {
            return std::stoll(str(), nullptr, 0);
        }

        unsigned long long u() const {
            return std::stoull(str(), nullptr, 0);
        }

        const AttributeValue& operator[](std::string name) const;
        AttributeValue& operator[](std::string name);

        void print(std::ostream& o, size_t indent=0, bool one_line=false) const;
        friend std::ostream& operator<<(std::ostream& o, const Value&v) {
            v.print(o);
            return o;
        }
    };
    class ValueVector {
        std::vector<Value> vec={};
        std::unordered_map<std::string, size_t> map={};
    public:
        void push_back(Value v);
        Value& get(size_t index) { return vec[index]; }
        Value& get(std::string index) { return vec[map.at(index)]; }
        bool has(std::string index) { return map.contains(index); }
        size_t size() { return vec.size(); }
    };
    struct AttributeValue {
        std::variant<Value, std::vector<Value>> data = std::vector<Value>{};

        void append(Value v) {
            TEXTX_ASSERT(std::holds_alternative<std::vector<Value>>(data), "error appending data:", v);
            std::get<std::vector<Value>>(data).push_back(v);
        }

        bool is_null() const {
            if (!std::holds_alternative<Value>(data)) {
                return false;
            }
            auto &value = std::get<Value>(data);
            return value.is_null();
        }

        bool is_boolean() const {
            if (!std::holds_alternative<Value>(data)) {
                return false;
            }
            auto &value = std::get<Value>(data);
            return std::holds_alternative<bool>(value.data);
        }

        bool is_ref() const {
            if (!std::holds_alternative<Value>(data)) {
                return false;
            }
            auto &value = std::get<Value>(data);
            return std::holds_alternative<ObjectRef>(value.data);
        }

        bool is_resolved() const {
            TEXTX_ASSERT(is_ref());
            return obj()!=nullptr;
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
            return std::holds_alternative<MatchText>(value.data);
        }
        bool is_number() const {
            if (!std::holds_alternative<Value>(data)) {
                return false;
            }
            auto &value = std::get<Value>(data);
            return value.is_number();
        }
        bool is_list() const {
            return std::holds_alternative<std::vector<Value>>(data);
        }

        bool boolean() const {
            TEXTX_ASSERT(std::holds_alternative<Value>(data));
            auto &value = std::get<Value>(data);
            TEXTX_ASSERT(std::holds_alternative<bool>(value.data), "no boolean");
            return std::get<bool>(value.data);
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

        std::string str() const {
            TEXTX_ASSERT(std::holds_alternative<Value>(data));
            auto &value = std::get<Value>(data);
            return value.str();
        }

        long double f() const {
            return std::stold(str());
        }

        long long i() const {
            return std::stoll(str(), nullptr, 0);
        }

        unsigned long long u() const {
            return std::stoull(str(), nullptr, 0);
        }

        std::vector<Value>::iterator begin();
        std::vector<Value>::iterator end();
        std::vector<Value>::const_iterator begin() const;
        std::vector<Value>::const_iterator end() const;
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
        std::weak_ptr<Object> weak_parent;
        textx::arpeggio::TextPosition pos;

        Object(std::shared_ptr<Object> parent, textx::arpeggio::TextPosition pos) : weak_parent{parent}, pos(pos) {}

        textx::arpeggio::TextPosition get_pos() { return pos; }
        std::shared_ptr<Object> parent() { return weak_parent.lock(); }
        std::shared_ptr<const Object> parent() const { return weak_parent.lock(); }
        std::shared_ptr<textx::Model> tx_model() const { return weak_model.lock(); }
        bool has_attr(std::string n) { return attributes.count(n)>0; }
        const AttributeValue& operator[](std::string name) const;
        AttributeValue& operator[](std::string name);
        void create_attribute_if_not_present(std::string name);
        bool is_instance(std::string base);

        void print(std::ostream& o, size_t indent=0, bool one_line=false) const;
        AttributeValue fqn_attributes(std::string name) const;
    };

    inline void traverse(textx::object::Value& v, std::function<void(textx::object::Value&)> f) {
        f(v);
        if (v.is_str()) {
            // nothing
        }
        else if (v.is_ref()) {
            // nothing
        }
        else if (v.is_boolean()) {
            // nothing
        }
        else if (v.is_null()) {
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
            textx::arpeggio::raise(v.pos, "unexpected situation (traversing the object)");
        }
    }
}