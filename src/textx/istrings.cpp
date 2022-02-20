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
            Command[skipws]: CommandObjAttributeAsString | CommandForLoop | CommandObj2StrFun;
            Data: Object | CommandForLoop; 
            CommandObjAttributeAsString: CMD_START obj=[Data] '.' fqn=FQN_WITH_ARRAY_ACCESS CMD_END;
            CommandForLoop:
                CMD_START 'FOR' name=ID ':' obj=[Data] '.' fqn=FQN_WITH_ARRAY_ACCESS
                body=CommandForLoopBody
                'ENDFOR' CMD_END
            ;
            CommandForLoopBody[noskipws]: CMD_END body=Model CMD_START;
            CommandObj2StrFun:
                 CMD_START call=[Function] "(" obj=[Data] (fqn=FqnObj)? ")" CMD_END
            ;
            FqnObj: '.' value=FQN_WITH_ARRAY_ACCESS;
            CMD_START: '{%';
            CMD_END: '%}';
            FQN_WITH_ARRAY_ACCESS: ID ('[' /\d+/ ']')? ('.' ID ('[' /\d+/ ']')?)*;
        )#");
        // TODO: prevent double vars!

        workspace->set_default_metamodel(workspace->get_metamodel_by_shortcut("ISTRINGS"));
        return workspace;
    }
}

using namespace textx::istrings::internal;

namespace textx::istrings {
    std::shared_ptr<textx::Workspace> get_new_istrings_metamodel_workspace() {
        std::shared_ptr<textx::Workspace> istrings_workspace = private_get_istrings_metamodel_workspace();
        istrings_workspace->get_metamodel_by_shortcut("ISTRINGS")->clear_builtin_models();
        return istrings_workspace;
    }

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
                else if (std::holds_alternative<textx::istrings::Obj2StrFun>(v)) {
                    s << "function " << k << "\n";
                }
                else {
                    throw std::runtime_error("unexpected: unknown external link type");
                }
            }
            mm->get_metamodel_by_shortcut("ISTRINGS")->add_builtin_model(
                mm->model_from_str("EXTERNAL_LINKAGE",s.str())
            );
        }        
        auto m = mm->model_from_str(istring_text);
        FormatterStream s;
        Formatter f{s, external_links};
        
        f.format_obj(m->val().obj());

        return s.str();
    }

}
