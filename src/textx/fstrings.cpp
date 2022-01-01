#include "textx/fstrings.h"

namespace textx::fstrings {
    std::shared_ptr<textx::Metamodel> get_fstrings_metamodel() {
        return textx::metamodel_from_str(R"#(
            Model[noskipws]: parts*=Part text*=Text;
            Part: text*=Text commandText=CommandText;
            Text: SpaceText|NormalText|NewlineText;
            NewlineText: text = /[\n]/;
            SpaceText: text = /[\t ]+/; // no newline
            NormalText: text = /([^{\s\n]|{[^%\s\n])([^{\n]|{[^%\n])*/;
            CommandText: '{%' command=Command '%}';
            Command[skipws]: CommandObjAttributeAsString;
            CommandObjAttributeAsString: todo=ID '.' attrName=ID;
        )#");
    }
}
