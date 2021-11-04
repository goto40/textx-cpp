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
        virtual void add_metamodel_from_file_for_extension(std::string n, std::filesystem::path filename) = 0;
        virtual void add_metamodel_from_str_for_extension(std::string n, std::filesystem::path filename, std::string grammar) = 0;
        virtual std::shared_ptr<textx::Model> model_from_file(std::filesystem::path filename, bool is_main_model=true) = 0;
        virtual void set_default_metamodel(std::shared_ptr<textx::Metamodel> m) = 0;
        virtual std::shared_ptr<textx::Model> model_from_str(std::string extension, std::string model_text) = 0;
        virtual std::shared_ptr<textx::Model> model_from_str(std::string model_text) = 0;
        virtual bool has_metamodelmodel_by_shortcut(std::string name) = 0;
        virtual std::shared_ptr<textx::Metamodel> get_metamodel_by_shortcut(std::string name) = 0;

        protected:
        friend Metamodel;
        virtual void add_metamodel_for_extension(std::string n, std::shared_ptr<textx::Metamodel> m) = 0;
        virtual void add_known_model(std::string path, std::shared_ptr<textx::Model> m) = 0; 
        virtual void add_known_metamodel(std::string path, std::shared_ptr<textx::Metamodel> m) = 0; 
        virtual bool has_model(std::string filename) = 0;
        virtual bool has_metamodelmodel(std::string filename) = 0;
        virtual std::shared_ptr<textx::Model> get_model(std::string filename) = 0;
        virtual std::shared_ptr<textx::Metamodel> get_metamodel(std::string filename) = 0;
        virtual std::shared_ptr<textx::Metamodel> metamodel_from_file(std::filesystem::path filename, std::optional<std::string> grammar=std::nullopt) = 0;
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
        std::unordered_map<std::string, std::shared_ptr<textx::Metamodel>> known_metamodels;
        std::unordered_map<std::string, std::shared_ptr<textx::Metamodel>> known_metamodels_by_shortcut;
        WorkspaceImpl() = default;
    public:
        static std::shared_ptr<WorkspaceImpl> create() { return std::shared_ptr<textx::WorkspaceImpl<Ptr4DefaultMM>>{new textx::WorkspaceImpl<Ptr4DefaultMM>{}}; }
        void set_default_metamodel(std::shared_ptr<textx::Metamodel> m) override {
            TEXTX_ASSERT(m!=nullptr, "default metamodel must not be a nullptr");
            default_metamodel=m;
        }
        void add_metamodel_for_extension(std::string n, std::shared_ptr<textx::Metamodel> m) override { extension_to_metamodel[n]=m; }
        void add_metamodel_from_file_for_extension(std::string n, std::filesystem::path filename) override {
            add_metamodel_for_extension(n, this->metamodel_from_file(filename));
        }
        void add_metamodel_from_str_for_extension(std::string n, std::filesystem::path filename, std::string grammar) override {
            add_metamodel_for_extension(n, this->metamodel_from_file(filename, grammar));
        }
        void add_known_model(std::string path, std::shared_ptr<textx::Model> m) override { 
            //std::cout << "adding " << path << "\n";
            known_models[path]=m;
        }
        void add_known_metamodel(std::string path, std::shared_ptr<textx::Metamodel> m) override { 
            known_metamodels[path]=m;
            std::string shortcut = std::filesystem::path(path).stem();
            //std::cout << "adding " << shortcut << "\n";
            known_metamodels_by_shortcut[shortcut]=m;
            //std::cout << shared_from_this() << "--> known_metamodels_by_shortcut.size()==" << known_metamodels_by_shortcut.size() << "\n";
        }
        bool has_model(std::string filename) override {
            return known_models.count(filename)>0;
        }
        bool has_metamodelmodel(std::string filename) override {
            return known_metamodels.count(filename)>0;
        }
        bool has_metamodelmodel_by_shortcut(std::string name) override {
            return known_metamodels_by_shortcut.count(name)>0;
        }

        std::shared_ptr<textx::Metamodel> get_metamodel(std::string filename) override {
            if (known_metamodels.count(filename)) {
                return known_metamodels[filename]; // cached model
            }
            else {
                return nullptr;
            }
        }
        std::shared_ptr<textx::Metamodel> get_metamodel_by_shortcut(std::string name) override {
            if (known_metamodels_by_shortcut.count(name)) {
                return known_metamodels_by_shortcut[name]; // cached model
            }
            else {
                //std::cout << shared_from_this() << " => known_metamodels_by_shortcut.size()==" << known_metamodels_by_shortcut.size() << "\n";
                // for (auto &[k,v]: known_metamodels_by_shortcut) {
                //     std::cout << "known: " << k << "\n";
                // }
                return nullptr;
            }
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
                throw std::runtime_error(std::string{"no metamodel available for "}+ending+", "+std::string{filename});
            }
        }
        std::shared_ptr<textx::Metamodel> metamodel_from_file(std::filesystem::path filename, std::optional<std::string> grammar=std::nullopt) override {
            if (!grammar.has_value()) {
                filename = std::filesystem::canonical(filename);
            }
            std::string shortcut = filename.stem();
            if (has_metamodelmodel(filename)) {
                TEXTX_ASSERT(has_metamodelmodel_by_shortcut(shortcut));
                return get_metamodel(filename);
            }
            else {
                TEXTX_ASSERT(!has_metamodelmodel_by_shortcut(shortcut), "names must be unique");
            }

            std::shared_ptr<textx::Metamodel> mm;
            if (grammar.has_value()) {
                mm = textx::metamodel_from_str(grammar.value(), filename, shared_from_this());
            }
            else {
                mm = textx::metamodel_from_file(filename, shared_from_this());
            }
            add_known_metamodel(filename, mm);
            return mm;
        }

        std::shared_ptr<textx::Model> model_from_str(std::string shortcut, std::string model_text) override {
            std::string fn = "";
            size_t p = shortcut.find('.');
            if (p!=shortcut.npos) {
                fn = shortcut;
                shortcut = shortcut.substr(p+1);
            }
            std::shared_ptr<textx::Metamodel> default_metamodel_ptr = nullptr;

            if constexpr (std::is_same_v<Ptr4DefaultMM<textx::Metamodel>, std::weak_ptr<textx::Metamodel> >) {
                default_metamodel_ptr = default_metamodel.lock();
            }
            else {
                default_metamodel_ptr = default_metamodel;
            }

            auto mm = get_metamodel_by_shortcut(shortcut);
            if (mm) {
                return mm->model_from_str(model_text,fn,true,shared_from_this());
            }
            else if (default_metamodel_ptr!=nullptr) {
                return default_metamodel_ptr->model_from_str(model_text,fn,true,shared_from_this());
            }
            else {
                throw std::runtime_error(std::string{"no metamodel available for metamodel "}+shortcut);
            }
        }
        std::shared_ptr<textx::Model> model_from_str(std::string model_text) override {
             std::shared_ptr<textx::Metamodel> default_metamodel_ptr = nullptr;

            if constexpr (std::is_same_v<Ptr4DefaultMM<textx::Metamodel>, std::weak_ptr<textx::Metamodel> >) {
                default_metamodel_ptr = default_metamodel.lock();
            }
            else {
                default_metamodel_ptr = default_metamodel;
            }

           if (default_metamodel_ptr!=nullptr) {
                return default_metamodel_ptr->model_from_str(model_text,"",true,shared_from_this());
            }
            else {
                throw std::runtime_error(std::string{"no default metamodel available"});
            }
        }
   };
}