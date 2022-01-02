#include "textx/workspace.h"
#include <unordered_map>
#include <variant>

namespace textx::fstrings {
    using ExternalLink = std::variant<std::string, std::shared_ptr<textx::object::Object>>;

    std::shared_ptr<textx::Workspace> get_fstrings_metamodel_workspace();
    std::string f(std::string fstring_text, std::unordered_map<std::string,ExternalLink> external_links);
}
