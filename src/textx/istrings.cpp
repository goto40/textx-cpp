#include "textx/istrings.h"

namespace {
    std::shared_ptr<textx::Workspace> private_get_istrings_metamodel_workspace() {
        auto workspace = textx::Workspace::create();

        workspace->add_metamodel_from_str_for_extension("ext", "EXTERNAL_LINKAGE.tx",
        R"#(
            Model: elements*=Element;
            Element: Object|Function;
            Object: 'object' name=ID;
            Function: 'function' name=ID;
        )#");

        workspace->add_metamodel_from_str_for_extension("istr", "ISTRINGS.tx",
        R"#(
            import EXTERNAL_LINKAGE
            Model[noskipws]: parts*=Part text*=Text;
            Part: text*=Text command=Command;
            Text: SpaceText|NormalText|NewlineText;
            NewlineText: text = /[\n]/;
            SpaceText: text = /[\t ]+/; // no newline
            NormalText: text = /([^{\s\n]|{[^%\s\n])([^{\n]|{[^%\n])*/;
            Command[skipws]: CommandObjAttributeAsString | CommandForLoop;
            Data: Object | CommandForLoop; 
            CommandObjAttributeAsString: CMD_START obj=[Data] '.' fqn=FQN CMD_END;
            CommandForLoop:
                CMD_START 'FOR' name=ID ':' obj=[Data] '.' fqn=FQN
                body=CommandForLoopBody
                'ENDFOR' CMD_END
            ;
            CommandForLoopBody[noskipws]:                 CMD_END body=Model CMD_START;
            CMD_START: '{%';
            CMD_END: '%}';
            FQN: ID ('.' ID)*;
        )#");
        // TODO: prevent double vars!

        workspace->set_default_metamodel(workspace->get_metamodel_by_shortcut("ISTRINGS"));
        return workspace;
    }
}

namespace {
    struct FormatterStream {
        std::ostringstream s;
        std::ostringstream line;
        size_t global_indent=0xFFFFFFFF;
        bool contains_nontextual_cmd = false;
        size_t current_pos_of_first_command=0xFFFFFFFF;
        template<class T>
        FormatterStream& operator<<(const T& x) {
            line << x;
            if (line.str().find('\n')!=line.str().npos) {
                consume();
            }
            return *this;
        }
        void consume() { // consume/process current line
            do {
                std::string l = line.str();
                line.str("");
                size_t idx = l.find('\n');
                if (idx!=l.npos) {
                    line << l.substr(idx+1);
                    l = l.substr(0, idx+1);
                    idx = l.find('\n');
                }
                if (contains_nontextual_cmd && line_is_empty(l)) {
                    // ok, ignore line
                }
                else {
                    if (line_is_empty(l)) {
                        l="\n";
                    }
                    if (l.size()>1) {
                        global_indent = std::min(global_indent, measure_indent(l));
                    }
                    s << l;
                }
                contains_nontextual_cmd = false;
                current_pos_of_first_command = 0xFFFFFFFF;
            } while (line.str().find('\n') != line.str().npos);
        }
        size_t measure_indent(const std::string &l) {
            for(size_t i=0;i<l.size();i++) {
                if (!std::isspace(l[i])) return i;
            }
            return l.size();
        }
        bool line_is_empty(const std::string &l) {
            for(size_t i=0;i<l.size();i++) {
                if (!std::isspace(l[i])) return false;
            }
            return true;
        }
        void nontextual_cmd() {
            // TODO memorize first pos
            if (current_pos_of_first_command==0xFFFFFFFF) {
                current_pos_of_first_command = line.str().size();
            }
            contains_nontextual_cmd = true;
        }
        std::string str() {
            consume();
            // a little hacky... remove global intend:
            std::ostringstream out;
            std::istringstream i{s.str()};
            std::string l;
            while( std::getline(i,l)) {
                if (l.size()>=global_indent) {
                    out << l.substr(global_indent) << "\n";
                }
                else {
                    out << "\n";
                }
            }
            l = out.str();
            l = l.substr(0,l.size()-1); // remove last newline
            return l;
        }
    };
    struct Formatter {
        FormatterStream &s;
        std::unordered_map<std::string,textx::istrings::ExternalLink> &external_links;
        std::unordered_map<std::string,std::shared_ptr<textx::object::Object>> loop_obj;

        std::shared_ptr<textx::object::Object> get_obj(std::shared_ptr<textx::object::Object> ref_obj) {
            if (ref_obj->type == "Object") {
                return std::get<std::shared_ptr<textx::object::Object>>(external_links[(*ref_obj)["name"].str()]);
            }
            if (ref_obj->type == "CommandForLoop") {
                auto name = (*ref_obj)["name"].str();
                if (loop_obj.count(name)>0) {
                    return loop_obj[name];
                }
                else {
                    throw std::runtime_error(std::string("expired loop ")+name);
                }
            }
            else {
                throw std::runtime_error(std::string("unknown obj link of type ")+ref_obj->type);
            }
        }

        void format_CommandObjAttributeAsString(std::shared_ptr<textx::object::Object> cmd) {
            auto obj = get_obj((*cmd)["obj"].obj());
            s << obj->fqn( (*cmd)["fqn"].str() ).str();
        }
        void format_CommandForLoop(std::shared_ptr<textx::object::Object> cmd) {
            auto name = (*cmd)["name"].str();
            auto obj = get_obj( (*cmd)["obj"].obj() );
            auto fqn_query = (*cmd)["fqn"].str();
            for(auto &e: obj->fqn(fqn_query)) {
                s.nontextual_cmd(); // for
                loop_obj[name] = e.obj();
                format_obj( (*cmd)["body"]["body"].obj() );
                s.nontextual_cmd(); // endfor
            }
            loop_obj.erase(name);
        }
        void format_cmd(std::shared_ptr<textx::object::Object> cmd) {
            if (cmd->type=="CommandObjAttributeAsString") format_CommandObjAttributeAsString(cmd);
            else if (cmd->type=="CommandForLoop") format_CommandForLoop(cmd);
            else {
                throw std::runtime_error(std::string("unexpected command type: ")+cmd->type);
            }
        }
        void format_text(std::shared_ptr<textx::object::Object> obj) {
            for(auto &t: (*obj)["text"]) {
                s << t["text"].str();
            }
        }
        void format_part(std::shared_ptr<textx::object::Object> part) {
            format_text(part);
            format_cmd((*part)["command"].obj());
        }
        void format_obj(std::shared_ptr<textx::object::Object> obj) {
            for(auto &p: (*obj)["parts"]) {
                format_part(p.obj());
            }
            format_text(obj);
        }
    };
}

namespace textx::istrings {
    std::shared_ptr<textx::Workspace> get_istrings_metamodel_workspace() {
        thread_local std::shared_ptr<textx::Workspace> istrings_workspace = private_get_istrings_metamodel_workspace();
        istrings_workspace->get_metamodel_by_shortcut("ISTRINGS")->clear_builtin_models();
        return istrings_workspace;
    }

    std::string i(std::string istring_text, std::unordered_map<std::string,ExternalLink> external_links) {
        auto mm = textx::istrings::get_istrings_metamodel_workspace();
        {
            std::ostringstream s;
            for (auto &[k,v]: external_links) {
                if (std::holds_alternative<std::shared_ptr<textx::object::Object>>(v)) {
                    s << "object " << k << "\n";
                }
            }
            mm->get_metamodel_by_shortcut("ISTRINGS")->add_builtin_model(
                mm->model_from_str("EXTERNAL_LINKAGE",s.str())
            );
        }        
        auto m = mm->model_from_str(istring_text);
        FormatterStream s;
        Formatter f{s, external_links};
        
        f.format_obj(m->val().obj());

        return s.str();
    }

}
