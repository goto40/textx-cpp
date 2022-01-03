#include "textx/workspace.h"
#include <unordered_map>
#include <variant>

namespace textx::istrings {
    using ExternalLink = std::variant<std::string, std::shared_ptr<textx::object::Object>>;

    std::shared_ptr<textx::Workspace> get_istrings_metamodel_workspace();
    std::string i(std::string istring_text, const std::unordered_map<std::string,ExternalLink> &external_links);
}
