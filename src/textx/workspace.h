#pragma once

#include "textx/metamodel.h"
#include "textx/model.h"
#include <unordered_map>
#include <filesystem>

namespace textx {

    class Workspace : public std::enable_shared_from_this<Workspace> {
        std::shared_ptr<textx::Metamodel> default_metamodel = nullptr;
        std::unordered_map<std::string, std::shared_ptr<textx::Metamodel>> extension_to_metamodel = {};
        std::unordered_map<std::string, std::shared_ptr<textx::Model>> known_models;
        Workspace() = default;
    public:
        static std::shared_ptr<Workspace> create() { return std::shared_ptr<textx::Workspace>{new Workspace{}}; }
        void set_default_metamodel(std::shared_ptr<textx::Metamodel> m) { default_metamodel=m; }
        void add_metamodel(std::string n, std::shared_ptr<textx::Metamodel> m) { extension_to_metamodel[n]=m; }
        void add_model(std::string path, std::shared_ptr<textx::Model> m) { 
            //std::cout << "adding " << path << "\n";
            known_models[path]=m;
        }
        bool has_model(std::string filename) {
            return known_models.count(filename)>0;
        }

        std::shared_ptr<textx::Model> get_model(std::string filename) {
            if (known_models.count(filename)) {
                return known_models[filename]; // cached model
            }
            else {
                return nullptr;
            }
        }
        std::shared_ptr<textx::Model> model_from_file(std::filesystem::path filename, bool is_main_model=true) {
            filename = std::filesystem::canonical(filename);
            if (has_model(filename.string())) {
                return get_model(filename.string()); // cached model
            }

            std::string ending = filename.extension();
            if (extension_to_metamodel.count(ending)>0) {
                return extension_to_metamodel[ending]->model_from_file(filename,is_main_model,shared_from_this());
            }
            else if (default_metamodel!=nullptr) {
                return default_metamodel->model_from_file(filename,is_main_model,shared_from_this());
            }
            else {
                throw std::runtime_error(std::string{"no metamodel available for "}+std::string{filename});
            }
        }
    };
}