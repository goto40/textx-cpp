#include "mgrep.h"
#include <textx/istrings.h>

namespace mgrep {
    MGrep::MGrep(std::string model) {
        workspace->add_metamodel_from_str_for_extension("mgrep", "MGREP.tx", model);
        workspace->set_default_metamodel(workspace->get_metamodel_by_shortcut("MGREP"));
 
        external_links = { 
            {"model", std::shared_ptr<textx::object::Object>{} },
            {"filename", [](std::shared_ptr<textx::object::Object> o) -> std::string {
                return o->tx_model()->tx_filename();
            }},
            {"line", [line=line](std::shared_ptr<textx::object::Object> o) -> std::string {
                return std::to_string(line);
            }},
            {"print", [](std::shared_ptr<textx::object::Object> o) -> std::string {
                std::ostringstream s;
                o->print(s, 0, true);
                return s.str();
            }},
        }; 

        std::ostringstream s;
        for (auto &[k,v]: external_links) {
            if (std::holds_alternative<std::shared_ptr<textx::object::Object>>(v)) {
                s << "object " << k << "\n";
            }
            else if (std::holds_alternative<textx::istrings::Obj2StrFun>(v)) {
                s << "function " << k << "\n";
            }
            else {
                throw std::runtime_error("unexpected: unknown external link type");
            }
        }
        auto mm = istr_workspace;
        mm->get_metamodel_by_shortcut("ISTRINGS")->add_builtin_model(
            mm->model_from_str("EXTERNAL_LINKAGE",s.str())
        );
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
        std::shared_ptr<textx::Model> model={};
        try {
            model = workspace->model_from_str(text);
            model->set_filename_info(filename);
        }
        catch(...) {
            line++;
            return std::nullopt;
        }    

        external_links["model"] = model->val().obj();
        textx::istrings::internal::FormatterStream s;
        textx::istrings::internal::Formatter f{s, external_links};
                f.format_obj(istr_cmd->val().obj());

        line++;
        return s.str();
    }
}