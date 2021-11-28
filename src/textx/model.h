#pragma once
#include "textx/object.h"
#include "textx/parsetree.h"

namespace textx::model {

    class Model {
        std::shared_ptr<textx::object::Object> root;
        textx::parsetree::ParseTree parsetree;

        Model(textx::parsetree::ParseTree parsetree);
    };
}