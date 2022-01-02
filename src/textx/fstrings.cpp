#include "textx/fstrings.h"

namespace textx::fstrings {
    std::shared_ptr<textx::Workspace> get_fstrings_metamodel() {
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
