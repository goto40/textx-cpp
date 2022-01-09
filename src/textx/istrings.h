#include "textx/workspace.h"
#include <unordered_map>
#include <variant>

namespace textx::istrings {
    using Obj2StrFun = std::function<std::string(std::shared_ptr<textx::object::Object>)>;
    using ExternalLink = std::variant<Obj2StrFun, std::shared_ptr<textx::object::Object>>;

    std::shared_ptr<textx::Workspace> get_istrings_metamodel_workspace();
    std::string i(std::string istring_text, std::unordered_map<std::string,ExternalLink> external_links);
}
