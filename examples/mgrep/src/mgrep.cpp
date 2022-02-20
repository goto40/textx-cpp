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
            line++;
            return false;
        }    
    }
    std::optional<std::string> MGrep::parse_and_transform(std::string text, std::string filename) {
        TEXTX_ASSERT(m_transform_command.has_value(), "you need a transform command (istring)");
        try {
            auto model = workspace->model_from_str(text);
            model->set_filename_info(filename);
            auto res = textx::istrings::i(
                m_transform_command.value(),
                { 
                    {"model", model->val().obj()},
                    {"filename", [](std::shared_ptr<textx::object::Object> o) -> std::string {
                        return o->tx_model()->tx_filename();
                    }},
                    {"line", [line=line](std::shared_ptr<textx::object::Object> o) -> std::string {
                        return std::to_string(line);
                    }}
                }
            );
            line++;
            return res;
        }
        catch(...) {
            line++;
            return std::nullopt;
        }    
    }
}