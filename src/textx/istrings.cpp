#include "textx/istrings.h"

namespace {
    std::shared_ptr<textx::Workspace> private_get_istrings_metamodel_workspace() {
        auto workspace = textx::Workspace::create();

        workspace->add_metamodel_from_str_for_extension("ext", "EXTERNAL_LINKAGE.tx",
        R"#(
            Model: elements*=Element;
            Element: Object|Function;
            Object: 'object' name=ID;
            Function: 'function' name=ID;
        )#");

        workspace->add_metamodel_from_str_for_extension("istr", "ISTRINGS.tx",
        R"#(
            import EXTERNAL_LINKAGE
            Model[noskipws]: parts*=Part text*=Text;
            Part: text*=Text command=Command;
            Text: SpaceText|NormalText|NewlineText;
            NewlineText: text = /[\n]/;
            SpaceText: text = /[\t ]+/; // no newline
            NormalText: text = /([^{\s\n]|{[^%\s\n])([^{\n]|{[^%\n])*/;
            Command[skipws]: CommandObjAttributeAsString | CommandForLoop;
            Data: Object | CommandForLoop; 
            CommandObjAttributeAsString: CMD_START obj=[Data] '.' fqn=FQN CMD_END;
            CommandForLoop:
                CMD_START 'FOR' name=ID ':' obj=[Data] '.' fqn=FQN CMD_END
                body=Model
                CMD_START 'ENDFOR' CMD_END
            ;
            CMD_START: '{%';
            CMD_END: '%}';
            FQN: ID ('.' ID)*;
        )#");
        // TODO: prevent double vars!

        workspace->set_default_metamodel(workspace->get_metamodel_by_shortcut("ISTRINGS"));
        return workspace;
    }
}

namespace {
    struct Formatter {
        std::ostringstream &s;
        std::unordered_map<std::string,textx::istrings::ExternalLink> &external_links;
        std::unordered_map<std::string,std::shared_ptr<textx::object::Object>> loop_obj;

        std::shared_ptr<textx::object::Object> get_obj(std::shared_ptr<textx::object::Object> ref_obj) {
            if (ref_obj->type == "Object") {
                return std::get<std::shared_ptr<textx::object::Object>>(external_links[(*ref_obj)["name"].str()]);
            }
            if (ref_obj->type == "CommandForLoop") {
                auto name = (*ref_obj)["name"].str();
                if (loop_obj.count(name)>0) {
                    return loop_obj[name];
                }
                else {
                    throw std::runtime_error(std::string("expired loop ")+name);
                }
            }
            else {
                throw std::runtime_error(std::string("unknown obj link of type ")+ref_obj->type);
            }
        }

        void format_CommandObjAttributeAsString(std::shared_ptr<textx::object::Object> cmd) {
            auto obj = get_obj((*cmd)["obj"].obj());
            s << obj->fqn( (*cmd)["fqn"].str() ).str();
        }
        void format_CommandForLoop(std::shared_ptr<textx::object::Object> cmd) {
            auto name = (*cmd)["name"].str();
            auto obj = get_obj( (*cmd)["obj"].obj() );
            auto fqn_query = (*cmd)["fqn"].str();
            for(auto &e: obj->fqn(fqn_query)) {
                loop_obj[name] = e.obj();
                format_obj( (*cmd)["body"].obj() );
            }
            loop_obj.erase(name);
        }
        void format_cmd(std::shared_ptr<textx::object::Object> cmd) {
            if (cmd->type=="CommandObjAttributeAsString") format_CommandObjAttributeAsString(cmd);
            else if (cmd->type=="CommandForLoop") format_CommandForLoop(cmd);
            else {
                throw std::runtime_error(std::string("unexpected command type: ")+cmd->type);
            }
        }
        void format_text(std::shared_ptr<textx::object::Object> obj) {
            for(auto &t: (*obj)["text"]) {
                s << t["text"].str();
            }
        }
        void format_part(std::shared_ptr<textx::object::Object> part) {
            format_text(part);
            format_cmd((*part)["command"].obj());
        }
        void format_obj(std::shared_ptr<textx::object::Object> obj) {
            for(auto &p: (*obj)["parts"]) {
                format_part(p.obj());
            }
            format_text(obj);
        }
    };
}

namespace textx::istrings {
    std::shared_ptr<textx::Workspace> get_istrings_metamodel_workspace() {
        thread_local std::shared_ptr<textx::Workspace> istrings_workspace = private_get_istrings_metamodel_workspace();
        istrings_workspace->get_metamodel_by_shortcut("ISTRINGS")->clear_builtin_models();
        return istrings_workspace;
    }

    std::string i(std::string istring_text, std::unordered_map<std::string,ExternalLink> external_links) {
        auto mm = textx::istrings::get_istrings_metamodel_workspace();
        {
            std::ostringstream s;
            for (auto &[k,v]: external_links) {
                if (std::holds_alternative<std::shared_ptr<textx::object::Object>>(v)) {
                    s << "object " << k << "\n";
                }
            }
            mm->get_metamodel_by_shortcut("ISTRINGS")->add_builtin_model(
                mm->model_from_str("EXTERNAL_LINKAGE",s.str())
            );
        }        
        auto m = mm->model_from_str(istring_text);
        std::ostringstream s;
        Formatter f{s, external_links};
        
        f.format_obj(m->val().obj());

        return s.str();
    }

}
