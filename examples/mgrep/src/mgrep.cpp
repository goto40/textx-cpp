#include "mgrep.h"
#include <textx/istrings.h>

namespace mgrep {
    MGrep::MGrep(std::string model) {
        workspace->add_metamodel_from_str_for_extension("mgrep", "MGREP.tx", model);
        workspace->set_default_metamodel(workspace->get_metamodel_by_shortcut("MGREP"));
    }
    bool MGrep::parse_and_store(std::string text) {
        try {
            auto model = workspace->model_from_str(text);
            m_matches.emplace_back(line, model);
            line++;
            return true;
        }
        catch(...) {
            return false;
        }    
    }
    std::optional<std::string> MGrep::parse_and_transform(std::string text) {
        TEXTX_ASSERT(m_transform_command.has_value(), "you need a transform command (istring)");
        try {
            auto model = workspace->model_from_str(text);
            auto res = textx::istrings::i(
                m_transform_command.value(),
                { {"model", model->val().obj()} }
            );
            return res;
        }
        catch(...) {
            return std::nullopt;
        }    
    }
}