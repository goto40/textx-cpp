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
    std::vector<Value>::iterator AttributeValue::begin() {
        TEXTX_ASSERT(std::holds_alternative<std::vector<Value>>(data));
        return std::get<std::vector<Value>>(data).begin();
    }
    std::vector<Value>::iterator AttributeValue::end() {
        TEXTX_ASSERT(std::holds_alternative<std::vector<Value>>(data));
        return std::get<std::vector<Value>>(data).end();
    }
    std::vector<Value>::const_iterator AttributeValue::begin() const {
        TEXTX_ASSERT(std::holds_alternative<std::vector<Value>>(data));
        return std::get<std::vector<Value>>(data).begin();
    }
    std::vector<Value>::const_iterator AttributeValue::end() const {
        TEXTX_ASSERT(std::holds_alternative<std::vector<Value>>(data));
        return std::get<std::vector<Value>>(data).end();
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

    void Object::create_attribute_if_not_present(std::string name) {
        if (attributes.count(name)==0) {
            attributes[name]={};
        }
    }

    void Value::print(std::ostream& o, size_t indent) const {
        if (is_pure_obj()) {
            obj()->print(o, indent);
        }
        else if (is_str()) {
            o << std::string(indent,' ') << str() << "\n";
        }
        else if (is_ref()) {
            o << std::string(indent,' ') << "-[ref]->" << ref().name << "\n";
        }
        else {
            throw std::runtime_error("unexpected");
        }
    }

    void Object::print(std::ostream& o, size_t indent) const {
        o << std::string(indent,' ') << this->type << "{\n";
        for(auto &[name,a]: attributes) {
            o << std::string(indent+2,' ') << name << "=\n";
            if (std::holds_alternative<Value>((*this)[name].data)) {
                std::get<Value>((*this)[name].data).print(o,indent+4);
            }
            else { // array
                for(size_t i=0;i<(*this)[name].size();i++) {
                    (*this)[name][i].print(o,indent+4);
                }
            }
        }
        o << std::string(indent,' ') << "}\n";
    }

}
