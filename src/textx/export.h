#pragma once
#include "textx/model.h"
#include <memory>
#include <ostream>

namespace textx {

    void save_as_simple_json(std::shared_ptr<textx::Model> model, bool save_all=true);
    void save_as_simple_json(std::shared_ptr<textx::Model> model, std::ostream &s);

}