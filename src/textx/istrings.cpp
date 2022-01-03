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
            reference EXTERNAL_LINKAGE
            Model[noskipws]: parts*=Part text*=Text;
            Part: text*=Text command=Command;
            Text: SpaceText|NormalText|NewlineText;
            NewlineText: text = /[\n]/;
            SpaceText: text = /[\t ]+/; // no newline
            NormalText: text = /([^{\s\n]|{[^%\s\n])([^{\n]|{[^%\n])*/;
            Command[skipws]: CommandObjAttributeAsString;
            CommandObjAttributeAsString: CMD_START obj=[EXTERNAL_LINKAGE.Object] '.' fqn=FQN CMD_END;
            CMD_START: '{%';
            CMD_END: '%}';
            FQN: ID ('.' ID)*;
        )#");

        workspace->set_default_metamodel(workspace->get_metamodel_by_shortcut("ISTRINGS"));
        return workspace;
    }
}

namespace {
    struct Formatter {
        std::ostringstream &s;
        std::unordered_map<std::string,textx::istrings::ExternalLink> &external_links;

        void format_CommandObjAttributeAsString(std::shared_ptr<textx::object::Object> cmd) {
            auto obj = std::get<std::shared_ptr<textx::object::Object>>(external_links[(*cmd)["obj"]["name"].str()]);
            s << obj->fqn( (*cmd)["fqn"].str() ).str();
        }
        void format_cmd(std::shared_ptr<textx::object::Object> cmd) {
            if (cmd->type=="CommandObjAttributeAsString") format_CommandObjAttributeAsString(cmd);
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
        f.format_text(m->val().obj());

        return s.str();
    }

}
