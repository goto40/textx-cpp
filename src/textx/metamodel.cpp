#include "textx/metamodel.h"
#include "textx/rule.h"
#include "textx/workspace.h"
#include "textx/arpeggio.h"
#include <cassert>
#include <unordered_set>

namespace textx {

    Metamodel::Metamodel(std::string_view grammar_text, bool include_basic_metamodel, std::string filename, std::shared_ptr<textx::Workspace> workspace)
        : default_workspace{textx::WorkspaceImpl<std::weak_ptr>::create()} // ownership! The default workspace uses *this --> weak_pointer, else we get a memory hole
    {
        try {
            if (workspace==nullptr) {
                workspace = default_workspace;
            }
            if (filename!="") {
                grammar_name = std::filesystem::path(filename).stem();
            }
            grammar_root = textx_grammar.parse_or_throw(grammar_text);
            auto& root = grammar_root.value();

            assert(root.name && "unexpected: no textx model loaded!!");
            assert(root.name.value()=="rule://textx_model" && "unexpected: no textx model loaded!!");

            auto &rules = root.children[1];
            //std::cout << "children: " << rules.children.size() << "\n";

            for (auto&r : rules.children) {
                auto &rule_name = r.children[0].captured.value();
                //std::cout << "r: " << rule_name << "\n";
                auto &rule_params = r.children[1];
                auto &rule_body = r.children[3];
                auto new_rule = Rule(*this, rule_name, rule_params, rule_body);
                grammar.add_rule(rule_name, new_rule);
            }

            if(include_basic_metamodel) {
                imported_models.push_back(get_basic_metamodel());
                imported_models_by_name["BUILTIN"]=get_basic_metamodel();
                //textx_grammar_parsetree.copy_rule_infos_from("", get_basic_metamodel()->textx_grammar_parsetree);
            }
            auto &imports_or_refs = root.children[0];
            for(auto &i: imports_or_refs.children) {
                auto import = i.search("rule://import_stm");
                if (import) {
                    std::string name = import->children[1].captured.value();
                    auto imported_mm = workspace->get_metamodel_by_shortcut(name);
                    if (imported_mm==nullptr) {
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
                        imported_mm = workspace->metamodel_from_file(p);
                    }
                    imported_models.push_back(imported_mm);
                    imported_models_by_name[name]=imported_mm;
                }
                auto ref = i.search("rule://reference_stm");
                if (ref) {
                    std::string name = ref->children[1].captured.value();
                    auto referenced_mm = workspace->get_metamodel_by_shortcut(name);
                    TEXTX_ASSERT(referenced_mm!=nullptr, "unexpected, language not found: ", name);
                    //std::cout << "ref. " << name << " " << referenced_mm << "\n";
                    referenced_models.push_back(referenced_mm);
                    referenced_models_by_name[name]=referenced_mm;
                    TEXTX_ASSERT(ref->children[2].children.size()==0, "no alias allowed for referenced languages (not implemented)");
                }
            } 

            // two phase creation: we need rules defined late for RREL scope providers:
            bool first = true;
            for (auto&r : rules.children) {
                auto &rule_name = r.children[0].captured.value();
                auto &rule_params = r.children[1];
                auto &rule_body = r.children[3];
                auto &new_rule = grammar[rule_name];
                if (first) {
                    grammar.set_main_rule(rule_name);
                    if (new_rule.tx_params().count("noskipws")) {
                        grammar.set_default_skipws(false);
                    }
                    else {
                        grammar.set_default_skipws(true);
                    }
                    first = false;
                }
                new_rule.post_process_created_rule(*this, rule_name, rule_params, rule_body);
            }

            // fill "all types"
            get_all_types(all_types);

            bool all_rule_types_resolved = false;
            size_t unresolved_rules=grammar.size();
            while(unresolved_rules>0) {
                size_t unresolved_rules_new = 0;
                for (auto&[name,r] : grammar) {
                    r.determine_rule_type_and_adjust_inh_by(*this);
                    if (r.m_type == RuleType::illegal) {
                        unresolved_rules_new++;
                    }
                }
                if (unresolved_rules_new==unresolved_rules) {
                    throw std::runtime_error("unexpected: rule types could not be resolved... (maybe cyclic inheritance detected)");
                }
                unresolved_rules = unresolved_rules_new;
            }

            for (auto&[name,r] : grammar) {
                r.adjust_attr_types(*this);
            }

            for (auto&[name,r] : grammar) {
                r.intern_arpeggio_rule_body = nullptr; // no dangling pointer, even if we decide to forget the parse tree now...
            }

            // comments
            if (has_rule("Comment", false)) { // false: do not look for referenced languages, only included grammars
                grammar.get_config().skip_text = textx::arpeggio::skip_text_functions::combine({
                    textx::arpeggio::skip_text_functions::skipws(),
                    textx::arpeggio::skip_text_functions::skip_pattern(operator[]("Comment"))
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
        if (name=="OBJECT") {
            return name;
        }
        size_t n = name.find(".");
        if (n!=name.npos && name.substr(0,n)==grammar_name) {
            name = name.substr(n+1);
            n=name.npos;
        }
        if (n!=name.npos) {
            auto res = imported_models_by_name.find(name.substr(0,n));
            if (res == imported_models_by_name.end()) {
                res = referenced_models_by_name.find(name.substr(0,n));
                TEXTX_ASSERT(res!=referenced_models_by_name.end(), "grammar ", name.substr(0,n), " not found.");
            }
            else {
                TEXTX_ASSERT(res!=imported_models_by_name.end(), "grammar ", name.substr(0,n), " not found.");
            }
            auto sp = res->second.lock();
            TEXTX_ASSERT(sp!=nullptr);
            if (sp->has_rule(name.substr(n+1), true)) {
                return sp->get_fqn_for_rule(name.substr(n+1));
            }
        }
        else {
            //std::cout << "looking for " << name << " in " << this->grammar_name << "\n";
            if (grammar.get_rules().count(name)>0) {
                //std::cout << "found " << name << " in my model\n";
                if (grammar_name.size()==0) {
                    return name;
                }
                else {
                    return std::string(grammar_name)+"."+name;
                }
            }
            for(auto p:imported_models) {
                auto sp = p.lock();
                TEXTX_ASSERT(sp!=nullptr);
                //std::cout << "looking for " << name << " in imported models\n";
                if (sp->has_rule(name, true)) {
                    //std::cout << "found " << name << " in imported models\n";
                    return sp->get_fqn_for_rule(name.substr(n+1));
                }
            }
            for(auto p:referenced_models) {
                auto sp = p.lock();
                TEXTX_ASSERT(sp!=nullptr);
                if (sp->has_rule(name, true)) {
                    return sp->get_fqn_for_rule(name.substr(n+1));
                }
            }
        }
        //std::cout << "failed search for " << name << "... in " << this->grammar_name << "\n";
        throw std::runtime_error(std::string("rule ")+name+" not found.");
    }

    bool Metamodel::has_rule(std::string name, bool allow_referenced_mm) const {
        size_t n = name.find(".");
        if (n!=name.npos && name.substr(0,n)==grammar_name) {
            name = name.substr(n+1);
            n=name.npos;
        }
        if (n!=name.npos) {
            auto res = imported_models_by_name.find(name.substr(0,n));
            if (res==imported_models_by_name.end() && allow_referenced_mm) {
                res = referenced_models_by_name.find(name.substr(0,n));
                TEXTX_ASSERT(res!=referenced_models_by_name.end(), "grammar ", name.substr(0,n), " not found.");
            }
            else {
                TEXTX_ASSERT(res!=imported_models_by_name.end(), "grammar ", name.substr(0,n), " not found.");
            }
            auto sp = res->second.lock();
            TEXTX_ASSERT(sp!=nullptr);
            if (sp->has_rule(name.substr(n+1), allow_referenced_mm)) {
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
                if (sp->has_rule(name, allow_referenced_mm)) {
                    return true;
                }
            }
            if (allow_referenced_mm) for(auto p:referenced_models) {
                auto sp = p.lock();
                TEXTX_ASSERT(sp!=nullptr);
                if (sp->has_rule(name, allow_referenced_mm)) {
                    return true;
                }
            }
        }
        return false;
    }

    std::shared_ptr<Metamodel> Metamodel::get_basic_metamodel() {
        #ifdef ARPEGGIO_USE_BOOST_FOR_REGEX
        static std::shared_ptr<Metamodel> mm{new Metamodel{R"(
            ID: /[^\d\W]\w*\b/;
            BOOL: /(True|true|False|false|0|1)\b/;
            INT: /[-+]?[0-9]+\b/;
            FLOAT: /[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?(?<=[\w\.])(?![\w\.])/;
            STRICTFLOAT: /[+-]?(((\d+\.(\d*)?|\.\d+)([eE][+-]?\d+)?)|((\d+)([eE][+-]?\d+)))(?<=[\w\.])(?![\w\.])/;
            STRING: /("(\\"|[^"])*")|('(\\'|[^'])*')/;
            NUMBER: STRICTFLOAT|INT;
            BASETYPE: NUMBER|FLOAT|BOOL|ID|STRING;
        )", false, "BUILTIN.tx"}};
        #else
        //TODO fixme
        static std::shared_ptr<Metamodel> mm{new Metamodel{R"(
            ID: /[^\d\W]\w*\b/;
            BOOL: /(True|true|False|false|0|1)\b/;
            INT: /[-+]?[0-9]+\b/;
            FLOAT: /[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?(?<=[\w\.])(?![\w\.])/;
            STRICTFLOAT: /[+-]?(((\d+\.(\d*)?|\.\d+)([eE][+-]?\d+)?)|((\d+)([eE][+-]?\d+)))(?<=[\w\.])(?![\w\.])/;
            STRING: /("(\\"|[^"])*")|('(\\'|[^'])*')/;
            NUMBER: STRICTFLOAT|INT;
            BASETYPE: NUMBER|FLOAT|BOOL|ID|STRING;
        )", false, "BUILTIN.tx"}};
        #endif
        return mm;
    }

    textx::arpeggio::Pattern Metamodel::ref(std::string name) {
        return [this,name](const textx::arpeggio::Config &config, textx::arpeggio::ParserState &text, textx::arpeggio::TextPosition pos) -> std::optional<textx::arpeggio::Match>
        {
            return find_rule(name, false)(config, text, pos);
        };
    }

    Rule& Metamodel::operator[](std::string name) {
        return find_rule(name, true);
    }
    Rule& Metamodel::find_rule(std::string name, bool allow_referenced_mm) {
        size_t n = name.find(".");
        if (n!=name.npos && name.substr(0,n)==grammar_name) {
            name = name.substr(n+1);
            n=name.npos;
        }
        if (n!=name.npos) {
            auto res = imported_models_by_name.find(name.substr(0,n));
            if (res==imported_models_by_name.end() && allow_referenced_mm) {
                res = referenced_models_by_name.find(name.substr(0,n));
                TEXTX_ASSERT(res!=referenced_models_by_name.end(), "grammar ", name.substr(0,n), " not found.");
            }
            else {
                TEXTX_ASSERT(res!=imported_models_by_name.end(), "grammar ", name.substr(0,n), " not found.");
            }
            auto sp = res->second.lock();
            TEXTX_ASSERT(sp!=nullptr);
            if (sp->has_rule(name.substr(n+1), allow_referenced_mm)) {
                return sp->find_rule(name.substr(n+1), allow_referenced_mm);
            }
        }
        else {
            if (grammar.get_rules().count(name)>0) {
                return grammar[name];
            }
            for(auto p:imported_models) {
                auto sp = p.lock();
                TEXTX_ASSERT(sp!=nullptr);
                if (sp->has_rule(name, allow_referenced_mm)) {
                   return sp->find_rule(name, allow_referenced_mm);
                }
            }
            if (allow_referenced_mm) for(auto p:referenced_models) {
                auto sp = p.lock();
                TEXTX_ASSERT(sp!=nullptr);
                if (sp->has_rule(name, allow_referenced_mm)) {
                    return sp->find_rule(name, allow_referenced_mm);
                }
            }
        }
        throw std::runtime_error(std::string("cannot find rule \"")+name+"\";");
    }

    const Rule& Metamodel::operator[](std::string name) const {
        return find_rule(name, true);
    }
    const Rule& Metamodel::find_rule(std::string name, bool allow_referenced_mm) const {
        size_t n = name.find(".");
        if (n!=name.npos && name.substr(0,n)==grammar_name) {
            name = name.substr(n+1);
            n=name.npos;
        }
        if (n!=name.npos) { // found prefix in PREFIX.POSTFIX
            auto res = imported_models_by_name.find(name.substr(0,n));
            if (res==imported_models_by_name.end() && allow_referenced_mm) {
                res = referenced_models_by_name.find(name.substr(0,n));
                TEXTX_ASSERT(res!=referenced_models_by_name.end(), "grammar ", name.substr(0,n), " not found.");
            }
            else {
                TEXTX_ASSERT(res!=imported_models_by_name.end(), "grammar ", name.substr(0,n), " not found.");
            }
            auto sp = res->second.lock();
            TEXTX_ASSERT(sp!=nullptr);
            if (sp->has_rule(name.substr(n+1), allow_referenced_mm)) {
                return sp->find_rule(name.substr(n+1), allow_referenced_mm);
            }
        }
        else { // no prefix
            if (grammar.get_rules().count(name)>0) {
                return grammar[name];
            }
            for(auto p:imported_models) {
                auto sp = p.lock();
                TEXTX_ASSERT(sp!=nullptr);
                if (sp->has_rule(name, allow_referenced_mm)) {
                    return sp->find_rule(name, allow_referenced_mm);
                }
            }
            if (allow_referenced_mm) for(auto p:referenced_models) {
                auto sp = p.lock();
                TEXTX_ASSERT(sp!=nullptr);
                if (sp->has_rule(name, allow_referenced_mm)) {
                    return sp->find_rule(name, allow_referenced_mm);
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

    bool Metamodel::is_instance(std::string special, std::string base) const {
        //std::cout << "isinstance " << special << "--" << base << "\n";
        auto fqn_special = get_fqn_for_rule(special);
        auto fqn_base = get_fqn_for_rule(base);
        if (fqn_special == fqn_base) {
            return true;
        }
        else if (fqn_base=="OBJECT") {
            return true;
        }
        return (operator[](base).tx_inh_by().count(special)>0);
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

    void Metamodel::get_all_types(std::unordered_set<std::string> &res) {
        for (auto &[name, rule]: grammar) {
            res.insert(name);
        }
        for (auto weak_other_mm: imported_models) {
            auto other_mm = weak_other_mm.lock();
            TEXTX_ASSERT(other_mm != nullptr);
            other_mm->get_all_types(res);
        }
        for (auto weak_other_mm: referenced_models) {
            auto other_mm = weak_other_mm.lock();
            TEXTX_ASSERT(other_mm != nullptr);
            other_mm->get_all_types(res);
        }
    }
}