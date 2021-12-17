#pragma once

#include "textx/metamodel.h"
#include "textx/model.h"
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <type_traits> // later: concepts for is_...

namespace textx {

    class Workspace : public std::enable_shared_from_this<Workspace> {
        public:
        static std::shared_ptr<Workspace> create();
        virtual ~Workspace() = default;
        virtual void set_default_metamodel(std::shared_ptr<textx::Metamodel> m) = 0;
        virtual void add_metamodel(std::string n, std::shared_ptr<textx::Metamodel> m) = 0;
        virtual void add_model(std::string path, std::shared_ptr<textx::Model> m) = 0; 
        virtual bool has_model(std::string filename) = 0;
        virtual std::shared_ptr<textx::Model> get_model(std::string filename) = 0;
        virtual std::shared_ptr<textx::Model> model_from_file(std::filesystem::path filename, bool is_main_model=true) = 0;
    };
    template< template<class> class Ptr4DefaultMM=std::shared_ptr>
        // requires 
        //     std::is_same<Ptr4DefaultMM<textx::Metamodel>, std::shared_ptr<textx::Metamodel> >
        //     || std::is_same<Ptr4DefaultMM<textx::Metamodel>, std::weak_ptr<textx::Metamodel> >
    class WorkspaceImpl : public virtual Workspace {
        static_assert(
            std::is_same_v<Ptr4DefaultMM<textx::Metamodel>, std::shared_ptr<textx::Metamodel> >
         || std::is_same_v<Ptr4DefaultMM<textx::Metamodel>, std::weak_ptr<textx::Metamodel> >,
         "used Workspace w/o proper pointer type for default Metamodel"
        );

        Ptr4DefaultMM<textx::Metamodel> default_metamodel = {};
        std::unordered_map<std::string, std::shared_ptr<textx::Metamodel>> extension_to_metamodel = {};
        std::unordered_map<std::string, std::shared_ptr<textx::Model>> known_models;
        WorkspaceImpl() = default;
    public:
        static std::shared_ptr<WorkspaceImpl> create() { return std::shared_ptr<textx::WorkspaceImpl<Ptr4DefaultMM>>{new textx::WorkspaceImpl<Ptr4DefaultMM>{}}; }
        void set_default_metamodel(std::shared_ptr<textx::Metamodel> m) override { default_metamodel=m; }
        void add_metamodel(std::string n, std::shared_ptr<textx::Metamodel> m) override { extension_to_metamodel[n]=m; }
        void add_model(std::string path, std::shared_ptr<textx::Model> m) override { 
            //std::cout << "adding " << path << "\n";
            known_models[path]=m;
        }
        bool has_model(std::string filename) override {
            return known_models.count(filename)>0;
        }

        std::shared_ptr<textx::Model> get_model(std::string filename) override {
            if (known_models.count(filename)) {
                return known_models[filename]; // cached model
            }
            else {
                return nullptr;
            }
        }
        std::shared_ptr<textx::Model> model_from_file(std::filesystem::path filename, bool is_main_model=true) override {
            filename = std::filesystem::canonical(filename);
            std::string ending = filename.extension();
            std::shared_ptr<textx::Metamodel> default_metamodel_ptr = nullptr;

            if constexpr (std::is_same_v<Ptr4DefaultMM<textx::Metamodel>, std::weak_ptr<textx::Metamodel> >) {
                default_metamodel_ptr = default_metamodel.lock();
            }
            else {
                default_metamodel_ptr = default_metamodel;
            }

            if (extension_to_metamodel.count(ending)>0) {
                return extension_to_metamodel[ending]->model_from_file(filename,is_main_model,shared_from_this());
            }
            else if (default_metamodel_ptr!=nullptr) {
                return default_metamodel_ptr->model_from_file(filename,is_main_model,shared_from_this());
            }
            else {
                throw std::runtime_error(std::string{"no metamodel available for "}+std::string{filename});
            }
        }
    };
}