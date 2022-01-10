#include "textx/workspace.h"
#include <unordered_map>
#include <variant>

namespace textx::istrings {
    using Obj2StrFun = std::function<std::string(std::shared_ptr<textx::object::Object>)>;
    using ExternalLink = std::variant<Obj2StrFun, std::shared_ptr<textx::object::Object>>;

    std::shared_ptr<textx::Workspace> get_istrings_metamodel_workspace();

    /**
     * @brief format an indented string
     * You can use <% obj.attr %> to print str-based attributes.
     * You can use <% FOR x: obj.lst %> ... <% ENDFOR %> (inspired by xtend).
     *  - Indentation is adapted and indentations introduced in for-loops are ignored.
     * You can use <% fun(obj) %> to call provided object-to-string conversion functions. 
     *  - Indentation is propagated into multi-line string outputs of the conversion functions. 
     * 
     * @param istring_text a string the the iformat language
     * @param external_links a map: name --> (shared_ptr<object>|string(shared_ptr<object>))
     * @return std::string 
     */
    std::string i(std::string istring_text, std::unordered_map<std::string,ExternalLink> external_links);
}
