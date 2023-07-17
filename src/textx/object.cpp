#include "textx/object.h"
#include "textx/metamodel.h"

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
        auto &v = std::get<std::vector<Value>>(data);
        TEXTX_ASSERT(idx<v.size(), "index out of bounds: index ", idx, " must be <", v.size());
        return v[idx];
    }
    Value& AttributeValue::operator[](size_t idx) {
        TEXTX_ASSERT(std::holds_alternative<std::vector<Value>>(data));
        auto &v = std::get<std::vector<Value>>(data);
        TEXTX_ASSERT(idx<v.size(), "index out of bounds: index ", idx, " must be <", v.size());
        return v[idx];
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

    bool Object::is_instance(std::string base) {
        return tx_model()->tx_metamodel()->is_instance(type, base);
    }

    namespace {
        auto extract_array_index(std::string name) {
            auto pos=name.find('[');
            std::optional<size_t> array_index=std::nullopt;
            if (pos!=std::string::npos) {
                TEXTX_ASSERT(name[name.size()-1]==']', " syntax error in array access attr[idx]: ", name);
                array_index = std::stoul(name.substr(pos+1,name.size()-pos-2));
                name = name.substr(0,pos);
            }
            return std::make_pair(name, array_index);
        }
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

    AttributeValue Object::fqn_attributes(std::string name) const {
        size_t pos = name.find('.');
        if (pos==name.npos) {
            auto [the_name, idx] = extract_array_index(name);
            if (idx.has_value()) {
                return AttributeValue{this->operator[](the_name)[idx.value()]};
            }
            else {
                return this->operator[](the_name);
            }
        }
        else {
            auto [the_name, idx] = extract_array_index(name.substr(0,pos));
            if (idx.has_value()) {
                return AttributeValue{this->operator[](the_name)[idx.value()].obj()->fqn_attributes(name.substr(pos+1))};
            }
            else {
                return this->operator[](the_name).obj()->fqn_attributes(name.substr(pos+1));
            }
        }
    }

    void Object::create_attribute_if_not_present(std::string name) {
        if (attributes.count(name)==0) {
            attributes[name]={};
        }
    }

    void Value::print(std::ostream& o, size_t indent, bool one_line) const {
        if (is_null()) {
            if (!one_line) o << std::string(indent,' ');
            o << "null";
            if (!one_line) o << "\n";
        }
        else if (is_pure_obj()) {
            obj()->print(o, indent, one_line);
        }
        else if (is_number()) {
            if (!one_line) o << std::string(indent,' ');
            o << f();
            if (!one_line) o << "\n";
        }
        else if (is_str()) {
            if (!one_line) o << std::string(indent,' ');
            o << "\"" << str() << "\"";
            if (!one_line) o << "\n";
        }
        else if (is_ref()) {
            if (!one_line) o << std::string(indent,' ');
            o << "-[ref]->" << ref().name << "(" << ref().obj.lock() << ")";
            if (!one_line) o << "\n";
        }
        else if (is_boolean()) {
            if (!one_line) o << std::string(indent,' ');
            o << boolean();
            if (!one_line) o << "\n";
        }
        else {
            throw std::runtime_error("unexpected, Value::print");
        }
    }

    void Object::print(std::ostream& o, size_t indent, bool one_line) const {
        if (!one_line) o << std::string(indent,' ');
        o << this->type << "{";
        if (!one_line) o << "\n";
        for(auto &[name,a]: attributes) {
            if (!one_line) o << std::string(indent+2,' ');
            o << name << "=";
            if (!one_line) o <<"\n";
            if (std::holds_alternative<Value>((*this)[name].data)) {
                std::get<Value>((*this)[name].data).print(o,indent+4, one_line);
            }
            else { // array
                for(size_t i=0;i<(*this)[name].size();i++) {
                    (*this)[name][i].print(o,indent+4, one_line);
                }
            }
        }
        if (!one_line) o << std::string(indent,' ');
        o << "}";
        if (!one_line) o << "\n";
    }

    void ValueVector::push_back(Value v) {
        if (v.is_obj() && v.obj()->has_attr("name")) {
            auto name = (*v.obj())["name"];
            if (name.is_str()) {
                map[name.str()] = vec.size();
            }
        }
        vec.push_back(v);
    }        
}
