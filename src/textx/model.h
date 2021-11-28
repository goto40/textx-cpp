#pragma once
#include <string>
#include <variant>
#include <vector>
#include <memory>
#include <unordered_map>

namespace textx::model {
    class Model;
    class Metamodel;
    class Object;

    struct ObjectRef {
        std::weak_ptr<Model> tx_model;
        std::string name;
        std::weak_ptr<Object> obj;
    };
    using Value = std::variant<std::string, std::shared_ptr<Object>, std::shared_ptr<ObjectRef>>;
    using AttributeValue = std::variant<Value, std::vector<Value>>;
    struct Object {
        std::string type;
        std::weak_ptr<Model> tx_model;
        std::unordered_map<std::string, Value> attributes;
    };
    class Model {
        std::weak_ptr<Metamodel> tx_model;
        std::shared_ptr<Object> root;
    };
}