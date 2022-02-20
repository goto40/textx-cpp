#include <textx/object.h>
#include <textx/workspace.h>
#include <textx/istrings.h>
#include <memory>
#include <deque>

namespace mgrep {
    struct MGrepMatch {
        std::size_t line;
        std::shared_ptr<textx::Model> model;
    };

    class MGrep {
        std::shared_ptr<textx::Workspace> workspace = textx::Workspace::create();
        std::shared_ptr<textx::Workspace> istr_workspace = textx::istrings::get_new_istrings_metamodel_workspace();
        std::shared_ptr<textx::Model> istr_cmd={};
        std::unordered_map<std::string,textx::istrings::ExternalLink> external_links={};
        size_t line = 1;
        std::deque<MGrepMatch> m_matches = {};
        std::optional<std::string> m_transform_command;
    public:
        MGrep(std::string model);
        void set_transform_command(std::string c) { 
            m_transform_command = c; 
            auto mm = istr_workspace;
            istr_cmd = mm->model_from_str(c);
       }
        bool parse_and_store(std::string text);
        std::optional<std::string> parse_and_transform(std::string text, std::string filename="");
        const auto &matches() { return m_matches; }
        void reset() {
            m_matches.clear(); line=1;
        }
    };
}