#pragma once
#include "textx/object.h"
#include "textx/parsetree.h"

namespace textx {

    class Model {
        std::weak_ptr<Metamodel> weak_mm;
        textx::object::Value root;
        textx::object::Value create_model(const std::string_view text, const textx::arpeggio::Match &m, textx::Metamodel &mm);
    public:
        Model(const std::string_view text, const textx::arpeggio::Match &parsetree, std::shared_ptr<Metamodel> mm);
    };
}