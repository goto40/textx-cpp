#include "mgrep.h"

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

}