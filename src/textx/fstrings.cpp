#include "textx/fstrings.h"

namespace {
    std::shared_ptr<textx::Workspace> private_get_fstrings_metamodel_workspace() {
        auto workspace = textx::Workspace::create();

        workspace->add_metamodel_from_str_for_extension("ext", "EXTERNAL_LINKAGE.tx",
        R"#(
            Model: elements*=Element;
            Element: Object|Function;
            Object: 'object' name=ID;
            Function: 'function' name=ID;
        )#");

        workspace->add_metamodel_from_str_for_extension("fstr", "FSTRINGS.tx",
        R"#(
            reference EXTERNAL_LINKAGE
            Model[noskipws]: parts*=Part text*=Text;
            Part: text*=Text command=Command;
            Text: SpaceText|NormalText|NewlineText;
            NewlineText: text = /[\n]/;
            SpaceText: text = /[\t ]+/; // no newline
            NormalText: text = /([^{\s\n]|{[^%\s\n])([^{\n]|{[^%\n])*/;
            Command[skipws]: CommandObjAttributeAsString;
            CommandObjAttributeAsString: CMD_START obj=[EXTERNAL_LINKAGE.Object] '.' attrName=FQN CMD_END;
            CMD_START: '{%';
            CMD_END: '%}';
            FQN: ID ('.' ID)*;
        )#");

        workspace->set_default_metamodel(workspace->get_metamodel_by_shortcut("FSTRINGS"));
        return workspace;
    }
}
namespace textx::fstrings {
    std::shared_ptr<textx::Workspace> get_fstrings_metamodel_workspace() {
        thread_local std::shared_ptr<textx::Workspace> fstrings_workspace = private_get_fstrings_metamodel_workspace();
        fstrings_workspace->get_metamodel_by_shortcut("FSTRINGS")->clear_builtin_models();
        return fstrings_workspace;
    }

    std::string f(std::string fstring_text, std::unordered_map<std::string,ExternalLink> external_links) {
        auto mm = textx::fstrings::get_fstrings_metamodel_workspace();
        std::ostringstream s;
        for (auto &[k,v]: external_links) {
            if (std::holds_alternative<std::shared_ptr<textx::object::Object>>(v)) {
                s << "object " << k << "\n";
            }
        }
        mm->get_metamodel_by_shortcut("FSTRINGS")->add_builtin_model(
            mm->model_from_str("EXTERNAL_LINKAGE",s.str())
        );
        auto m = mm->model_from_str(fstring_text);
        return "";
    }

}
