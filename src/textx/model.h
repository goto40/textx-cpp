#pragma once
#include "textx/object.h"
#include "textx/parsetree.h"

namespace textx {

    class Model {
        textx::object::Value root;
        std::weak_ptr<Metamodel> weak_mm;
        textx::object::Value create_model(const textx::arpeggio::Match &m, textx::Metamodel &mm);
    public:
        Model(const textx::arpeggio::Match &parsetree, std::shared_ptr<Metamodel> mm);
    };
}