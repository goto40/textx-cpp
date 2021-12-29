#include "textx/metamodel.h"
#include "textx/rule.h"
#include "textx/workspace.h"
#include <cassert>
#include <unordered_set>

namespace textx {

    Metamodel::Metamodel(std::string_view grammar_text, bool is_main_grammar, bool include_basic_metamodel, std::string filename, std::shared_ptr<textx::Workspace> workspace)
        : default_workspace{textx::WorkspaceImpl<std::weak_ptr>::create()} // ownership! The default workspace uses *this --> weak_pointer, else we get a memory hole
    {
        try {
            if (workspace==nullptr) {
                workspace = default_workspace;
            }
            if (filename!="") {
                grammar_name = std::filesystem::path(filename).stem();
            }
            textx_grammar_parsetree.root = textx_grammar.parse_or_throw(grammar_text);
            auto &root = textx_grammar_parsetree.root.value();

            assert(root.name && "unexpected: no textx model loaded!!");
            assert(root.name.value()=="rule://textx_model" && "unexpected: no textx model loaded!!");

            //TODO import and refs
                    
            auto &rules = root.children[1];
            //std::cout << "children: " << rules.children.size() << "\n";

            for (auto&r : rules.children) {
                auto &rule_name = r.children[0].captured.value();
                //std::cout << "r: " << rule_name << "\n";
                auto &rule_params = r.children[1];
                auto &rule_body = r.children[3];
                auto rule_info = textx::parsetree::RuleInfo{r,rule_name};
                auto new_rule = Rule(*this, rule_name, rule_params, rule_body, rule_info);
                grammar.add_rule(rule_name, new_rule);
                textx_grammar_parsetree.rule_info.emplace(rule_name, rule_info);
            }

            if(include_basic_metamodel) {
                imported_models.push_back(get_basic_metamodel());
                imported_models_by_name["BUILTIN"]=get_basic_metamodel();
                textx_grammar_parsetree.copy_rule_infos_from("", get_basic_metamodel()->textx_grammar_parsetree);
            }
            auto &imports_or_refs = root.children[0];
            for(auto &i: imports_or_refs.children) {
                auto import = i.search("rule://import_stm");
                if (import) {
                    std::string name = import->children[1].captured.value();
                    TEXTX_ASSERT(filename.size()>0);

                    auto basedir = std::filesystem::path(filename).parent_path();
                    auto basedir0 = std::filesystem::path(".");
                    auto import_filename = name+".tx";

                    auto p = basedir/import_filename;
                    if (!std::filesystem::exists(p)) {
                        p = basedir0/import_filename;
                    }
                    if (!std::filesystem::exists(p)) {
                        textx::arpeggio::raise(import->children[1].start(), import_filename+" not found.");
                    }
                    auto imported_mm = workspace->metamodel_from_file(p);
                    imported_models.push_back(imported_mm);
                    imported_models_by_name[name]=imported_mm;
                    textx_grammar_parsetree.copy_rule_infos_from(name, imported_mm->textx_grammar_parsetree);
                }
            } 

            // two phase creation: we need rules defined late for RREL scope providers:
            bool first = true;
            for (auto&r : rules.children) {
                auto &rule_name = r.children[0].captured.value();
                auto &rule_params = r.children[1];
                auto &rule_body = r.children[3];
                auto &rule_info = textx_grammar_parsetree[rule_name];
                bool add_eof = first && is_main_grammar;
                auto &new_rule = grammar[rule_name];
                if (first) {
                    grammar.set_main_rule(rule_name);
                    first = false;
                }
                new_rule.post_process_created_rule(*this, rule_name, rule_params, rule_body, rule_info, add_eof);
            }

            textx_grammar_parsetree.finalize_rule_info();

            for (auto&[name,r] : textx_grammar_parsetree.rule_info) {
                if (!r.external_rule) {
                    auto &rule = grammar[name];
                    rule.m_type = r.rule_type;
                    rule.m_tx_bases = r.tx_bases;
                    for(auto& [name, info]: r.attribute_info) {
                        rule.attribute_info[name].type = info.type;
                        rule.attribute_info[name].cardinality = r.get_attribute_cardinality(name);
                    }
                }
            }

            // comments
            if (has_rule("Comment")) {
                grammar.get_config().skip_text = textx::arpeggio::skip_text_functions::combine({
                    textx::arpeggio::skip_text_functions::skipws(),
                    textx::arpeggio::skip_text_functions::skip_pattern(grammar["Comment"])
                });
            }
        }
        catch(textx::arpeggio::Exception &e) {
            if (e.filename.size()==0) {
                textx::arpeggio::raise(std::string{grammar_text},filename, e.pos, e.error);   
            }
            else {
                throw;
            }
        }
        catch(std::exception &e) {
            throw std::runtime_error(filename+": "+e.what());
        }
    }

    std::string Metamodel::get_fqn_for_rule(std::string name) const {
        size_t n = name.find(".");
        if (n!=name.npos && name.substr(0,n)==grammar_name) {
            name = name.substr(n+1);
            n=name.npos;
        }
        if (n!=name.npos) {
            auto res = imported_models_by_name.find(name.substr(0,n));
            TEXTX_ASSERT(res!=imported_models_by_name.end(), "grammar ", name.substr(0,n), " not found.");
            auto sp = res->second.lock();
            TEXTX_ASSERT(sp!=nullptr);
            if (sp->has_rule(name.substr(n+1))) {
                return sp->get_fqn_for_rule(name.substr(n+1));
            }
        }
        else {
            if (grammar.get_rules().count(name)>0) {
                return std::string(grammar_name)+"."+name;
            }
            for(auto p:imported_models) {
                auto sp = p.lock();
                TEXTX_ASSERT(sp!=nullptr);
                if (sp->has_rule(name)) {
                    return sp->get_fqn_for_rule(name.substr(n+1));
                }
            }
        }
        throw std::runtime_error(std::string("rule ")+name+" not found.");
    }

    bool Metamodel::has_rule(std::string name) const {
        size_t n = name.find(".");
        if (n!=name.npos && name.substr(0,n)==grammar_name) {
            name = name.substr(n+1);
            n=name.npos;
        }
        if (n!=name.npos) {
            auto res = imported_models_by_name.find(name.substr(0,n));
            TEXTX_ASSERT(res!=imported_models_by_name.end(), "grammar ", name.substr(0,n), " not found.");
            auto sp = res->second.lock();
            TEXTX_ASSERT(sp!=nullptr);
            if (sp->has_rule(name.substr(n+1))) {
                return true;
            }
        }
        else {
            if (grammar.get_rules().count(name)>0) {
                return true;
            }
            for(auto p:imported_models) {
                auto sp = p.lock();
                TEXTX_ASSERT(sp!=nullptr);
                if (sp->has_rule(name)) {
                    return true;
                }
            }
        }
        return false;
    }

    std::shared_ptr<Metamodel> Metamodel::get_basic_metamodel() {
        static std::shared_ptr<Metamodel> mm{new Metamodel{R"(
            ID: /[^\d\W]\w*\b/;
            BOOL: /(True|true|False|false|0|1)\b/;
            INT: /[-+]?[0-9]+\b/;
            FLOAT: /[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?(?<=[\w\.])(?![\w\.])/;
            STRICTFLOAT: /[+-]?(((\d+\.(\d*)?|\.\d+)([eE][+-]?\d+)?)|((\d+)([eE][+-]?\d+)))(?<=[\w\.])(?![\w\.])/;
            STRING: /("(\\"|[^"])*")|('(\\'|[^'])*')/;
            NUMBER: STRICTFLOAT|INT;
            BASETYPE: NUMBER|FLOAT|BOOL|ID|STRING;
        )", false, false, "BUILTIN.tx"}};
        return mm;
    }

    textx::arpeggio::Pattern Metamodel::ref(std::string name) {
        return [this,name](const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) -> std::optional<textx::arpeggio::Match>
        {
            return operator[](name)(config, text, pos);
        };
    }

    Rule& Metamodel::operator[](std::string name) {
        size_t n = name.find(".");
        if (n!=name.npos && name.substr(0,n)==grammar_name) {
            name = name.substr(n+1);
            n=name.npos;
        }
        if (n!=name.npos) {
            auto res = imported_models_by_name.find(name.substr(0,n));
            TEXTX_ASSERT(res!=imported_models_by_name.end(), "grammar ", name.substr(0,n), " not found.");
            auto sp = res->second.lock();
            TEXTX_ASSERT(sp!=nullptr);
            if (sp->has_rule(name.substr(n+1))) {
                return sp->operator[](name.substr(n+1));
            }
        }
        else {
            if (grammar.get_rules().count(name)>0) {
                return grammar[name];
            }
            for(auto p:imported_models) {
                auto sp = p.lock();
                TEXTX_ASSERT(sp!=nullptr);
                if (sp->has_rule(name)) {
                   return sp->operator[](name);
                }
            }
        }
        throw std::runtime_error(std::string("cannot find rule \"")+name+"\";");
    }

    const Rule& Metamodel::operator[](std::string name) const {
        size_t n = name.find(".");
        if (n!=name.npos && name.substr(0,n)==grammar_name) {
            name = name.substr(n+1);
            n=name.npos;
        }
        if (n!=name.npos) {
            auto res = imported_models_by_name.find(name.substr(0,n));
            TEXTX_ASSERT(res!=imported_models_by_name.end(), "grammar ", name.substr(0,n), " not found.");
            auto sp = res->second.lock();
            TEXTX_ASSERT(sp!=nullptr);
            if (sp->has_rule(name.substr(n+1))) {
                return sp->operator[](name.substr(n+1));
            }
        }
        else {
            if (grammar.get_rules().count(name)>0) {
                return grammar[name];
            }
            for(auto p:imported_models) {
                auto sp = p.lock();
                TEXTX_ASSERT(sp!=nullptr);
                if (sp->has_rule(name)) {
                    return sp->operator[](name);
                }
            }
        }
        throw std::runtime_error(std::string("cannot find rule \"")+name+"\";");
    }

    namespace {
        // collect all imported models recursively
        inline void find_all_imported_models(std::shared_ptr<textx::Model> m, std::unordered_set<std::shared_ptr<textx::Model>>& result) {
            if (result.count(m)==0) {
                result.insert(m);
                for (auto p: m->tx_imported_models()) {
                    find_all_imported_models(p.lock(), result);
                }
            }
        }
    }

    std::shared_ptr<textx::Workspace> Metamodel::tx_default_workspace() {
        default_workspace->set_default_metamodel(shared_from_this()); 
        return default_workspace;
    }

    std::shared_ptr<textx::Model> Metamodel::model_from_str(std::string_view text, std::string filename, bool is_main_model, std::shared_ptr<textx::Workspace> workspace) {
        try {
            if (workspace==nullptr) {
                workspace = tx_default_workspace();
            }
            if (workspace->has_model(filename)) {
                return workspace->get_model(filename); // cached model
            }

            auto parsetree = parsetree_from_str(text);
            auto ret=std::shared_ptr<textx::Model>{new textx::Model()}; // call private constructor (new)
            //std::cout << parsetree.value() << "\n";
            ret->init(filename, text, *parsetree, shared_from_this());

            if (filename.size()>0) {
                workspace->add_known_model(filename, ret); // owning...
            }

            auto basedir = std::filesystem::path(filename).parent_path();
            auto basedir0 = std::filesystem::path(".");
            textx::object::traverse(ret->val(), [&, this](textx::object::Value& v) {
                if (v.is_pure_obj() && !v.is_null() && v.obj()->has_attr("importURI")) {
                    TEXTX_ASSERT(v["importURI"].is_str(), "importURI must be a string");
                    auto import_filename = v["importURI"].str();
                    auto p = basedir/import_filename;
                    if (!std::filesystem::exists(p)) {
                        p = basedir0/import_filename;
                    }
                    if (!std::filesystem::exists(p)) {
                        textx::arpeggio::raise(v.obj()->pos, import_filename+" not found.");
                    }
                    auto m = workspace->model_from_file(p, false); // unresolved refs
                    ret->add_imported_model(m);
                }
            });
 
            for (auto& builtin_model: builtin_models) {
                ret->add_imported_model(builtin_model);
            }

            if (is_main_model) {
                std::unordered_set<std::shared_ptr<Model>> all_models;
                find_all_imported_models(ret, all_models);

                size_t last_unresolved_refs=0;
                size_t unresolved_refs=0;
                do {
                    last_unresolved_refs=unresolved_refs;
                    unresolved_refs=0;
                    for(auto m: all_models) {
                        unresolved_refs += m->resolve_references();
                    }
                } while(unresolved_refs>0 && unresolved_refs!=last_unresolved_refs);

                if (unresolved_refs>0) {
                    textx::arpeggio::TextPosition pos;
                    std::shared_ptr<textx::Model> err_model;
                    std::stringstream error_text;
                    for(auto m: all_models) {
                        textx::object::traverse(m->val(),[&](textx::object::Value& v) {
                            if (v.is_ref() && !v.ref().obj.lock()) {
                                error_text << "ref '" << v.ref().name << "' not found at " << v.pos << ";\n";
                                pos = v.pos;
                                err_model = m;
                            }
                        });
                    }
                    textx::arpeggio::raise(err_model->tx_text(), err_model->tx_filename(), pos, error_text.str());
                }
            }
            return ret;
        }
        catch(textx::arpeggio::Exception &e) {
            if (e.filename.size()>0) {
                throw;
            }
            else {
                raise(std::string(text), filename, e.pos, e.error);
            }
        }
        catch(std::exception &e) {
            throw std::runtime_error(filename+": "+e.what());
        }
    }

    std::shared_ptr<textx::Model> Metamodel::model_from_file(std::filesystem::path p, bool is_main_model, std::shared_ptr<textx::Workspace> workspace) {
        std::ifstream file(p);
        std::stringstream modeltext;
        modeltext << file.rdbuf();
        auto m = model_from_str(modeltext.str(), p.string(), is_main_model, workspace);
        return m;
    }

    bool Metamodel::is_base_of(std::string base, std::string special) const {
        //std::cout << "is_base_of " << base << " " << special << "?\n";
        
        // special case:
        if (base=="OBJECT") {
            return true;
        }
        TEXTX_ASSERT(has_rule(base));
        TEXTX_ASSERT(has_rule(special));
        if ( base==special) return true;
        else if (operator[](special).tx_bases().size()>0) {
            for (auto b: operator[](special).tx_bases()) {
                auto res = is_base_of(base,b);
                if (res) return res;
            }
        }
        return false;
    }
}