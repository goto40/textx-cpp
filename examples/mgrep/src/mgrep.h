#include <textx/object.h>
#include <textx/workspace.h>
#include <memory>
#include <deque>

namespace mgrep {
    struct MGrepMatch {
        std::size_t line;
        std::shared_ptr<textx::Model> model;
    };

    class MGrep {
        std::shared_ptr<textx::Workspace> workspace = textx::Workspace::create();;
        size_t line = 1;
        std::deque<MGrepMatch> m_matches = {};
        std::optional<std::string> m_transform_command;
    public:
        MGrep(std::string model);
        void set_transform_command(std::string c) { m_transform_command=c; }
        bool parse_and_store(std::string text);
        std::optional<std::string> parse_and_transform(std::string text);
        const auto &matches() { return m_matches; }
    };
}