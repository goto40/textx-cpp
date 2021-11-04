#pragma once
#include "textx/model.h"
#include <memory>
#include <ostream>

namespace textx {

    /**
     * @brief exports a model to a json file
     * 
     * @param model the model to be exported
     * @param save_all also export all referenced models
     * @param schema_url use this schema url (if empty a schema is generated locally)
     */
    void save_model_as_json(std::shared_ptr<textx::Model> model, bool save_all=true, std::string schema_url="", std::string dest_str="src-gen");
    void save_model_as_json(std::shared_ptr<textx::Model> model, std::ostream &s, std::string schema_url="");
    void save_metamodel_as_json_schema(std::shared_ptr<textx::Metamodel> mm, bool save_all=true, std::string url_prefix="./", std::string dest_str="src-gen");
    void save_metamodel_as_json_schema(std::shared_ptr<textx::Metamodel> mm, std::ostream &s, std::string url_prefix="./");
}