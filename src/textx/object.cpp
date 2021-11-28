#include "object.h"

namespace textx::object {

    const AttributeValue& Value::operator[](std::string name) const {
        return (*obj())[name];
    }
    AttributeValue& Value::operator[](std::string name) {
        return (*obj())[name];
    }

    const AttributeValue& AttributeValue::operator[](std::string name) const {
        return (*obj())[name];
    }
    AttributeValue& AttributeValue::operator[](std::string name) {
        return (*obj())[name];
    }

    const Value& AttributeValue::operator[](size_t idx) const {
        TEXTX_ASSERT(std::holds_alternative<std::vector<Value>>(data));
        return std::get<std::vector<Value>>(data)[idx];
    }
    Value& AttributeValue::operator[](size_t idx) {
        TEXTX_ASSERT(std::holds_alternative<std::vector<Value>>(data));
        return std::get<std::vector<Value>>(data)[idx];
    }
    size_t AttributeValue::size() const {
        TEXTX_ASSERT(std::holds_alternative<std::vector<Value>>(data));
        return std::get<std::vector<Value>>(data).size();
    }


    const AttributeValue& Object::operator[](std::string name) const {
        auto r = attributes.find(name);
        if (r==attributes.end()) {
            throw std::runtime_error(std::string("attribute ")+name+" not found.");
        } 
        return r->second;
    }
    
    AttributeValue& Object::operator[](std::string name) {
        auto r = attributes.find(name);
        if (r==attributes.end()) {
            throw std::runtime_error(std::string("attribute ")+name+" not found.");
        } 
        return r->second;
    }
}
