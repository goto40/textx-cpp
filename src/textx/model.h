#pragma once
#include "textx/object.h"
#include "textx/parsetree.h"

namespace textx::model {

    class Model {
        textx::object::Value root;
        textx::object::Value create_model(const textx::arpeggio::Match &m);
    public:
        Model(const textx::arpeggio::Match &parsetree);
    };
}