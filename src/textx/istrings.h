#pragma once 
#include "textx/workspace.h"
#include <unordered_map>
#include <variant>
#include <stack>

#define DBG(x)

namespace textx::istrings {

    using Obj2StrFun = std::function<std::string(std::shared_ptr<textx::object::Object>)>;
    using ExternalLink = std::variant<Obj2StrFun, std::shared_ptr<textx::object::Object>>;

    std::shared_ptr<textx::Workspace> get_istrings_metamodel_workspace();
    std::shared_ptr<textx::Workspace> get_new_istrings_metamodel_workspace();

    /**
     * @brief format an indented string
     * You can use <% obj.attr %> to print str-based attributes.
     * You can use <% FOR x: obj.lst %> ... <% ENDFOR %> (inspired by xtend).
     *  - Indentation is adapted and indentations introduced in for-loops are ignored.
     * You can use <% fun(obj) %> to call provided object-to-string conversion functions. 
     *  - Indentation is propagated into multi-line string outputs of the conversion functions. 
     * 
     * @param istring_text a string the the iformat language
     * @param external_links a map: name --> (shared_ptr<object>|string(shared_ptr<object>))
     * @return std::string 
     */
    std::string i(std::string istring_text, std::unordered_map<std::string,ExternalLink> external_links);


    namespace internal {
        struct Line {
            std::string text;
            size_t indent;
            size_t nest_level;
            bool ignore=false;
            bool is_empty() { return text.empty(); }
        };
        struct FormatterStream {
            std::vector<Line> lines;
            std::ostringstream line;
            size_t global_indent=0xFFFFFFFF;
            bool contains_nontextual_cmd = false;
            size_t current_pos_of_first_command=0xFFFFFFFF;
            size_t nest_level=0;
            void inc_level() { nest_level++; }
            void dec_level() { TEXTX_ASSERT(nest_level>0); nest_level--; }
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
                        l = l.substr(0, idx);
                        idx = l.find('\n');
                    }
                    if (contains_nontextual_cmd && line_is_empty(l)) {
                        // ok, ignore line
                        size_t indent=0;
                        if (contains_nontextual_cmd) {
                            indent = current_pos_of_first_command;
                        }
                        lines.emplace_back("",indent, nest_level, true);
                    }
                    else {
                        size_t indent=0;
                        if (line_is_empty(l)) {
                            l="";
                        }
                        if (l.size()>1) {
                            indent = measure_indent(l);
                            global_indent = std::min(global_indent, indent);
                            l = l.substr(indent);
                        }
                        lines.emplace_back(l,indent, nest_level, false);
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
                std::ostringstream out;
                size_t prev_level = 0;
                size_t intend_correction = 0;
                size_t base_intend_for_next_correction_adaptation = 0;
                size_t activate_next_correction_adaptation = 0;
                std::stack<size_t> nested_intend_correction;
                for (auto& li: lines) {
                    size_t intend = 0;
                    if (li.indent>=global_indent) {
                        intend = li.indent-global_indent;
                    }
                    size_t concrete_intend = intend;
                    if (concrete_intend>=intend_correction) {
                        concrete_intend -= intend_correction;
                    }

                    if (activate_next_correction_adaptation>0) {
                        // update correction
                        activate_next_correction_adaptation=0;
                        if (intend>=base_intend_for_next_correction_adaptation) {
                            size_t corr = intend-base_intend_for_next_correction_adaptation;
                            intend_correction += corr;
                        }
                        else {
                            size_t corr = base_intend_for_next_correction_adaptation-intend;
                            if (intend_correction>=corr) {
                                intend_correction -= corr;
                            }
                            else {
                                intend_correction = 0;
                            }
                        }

                        concrete_intend = intend;
                        if (concrete_intend>=intend_correction) {
                            concrete_intend -= intend_correction;
                        }
                    }

                    if (prev_level<li.nest_level) {
                        nested_intend_correction.push(intend_correction);
                        if (activate_next_correction_adaptation==0) { // use first occurrence (of a "for") in line as ref
                            base_intend_for_next_correction_adaptation = intend;
                        }
                        activate_next_correction_adaptation++;
                    }
                    else if (prev_level>li.nest_level) {
                        TEXTX_ASSERT(nested_intend_correction.size()>0);
                        intend_correction = nested_intend_correction.top();
                        nested_intend_correction.pop();
                        if (activate_next_correction_adaptation>0) activate_next_correction_adaptation--;
                    }
                    if (!li.ignore) {
                        out << std::string(concrete_intend, ' ') << li.text
                            << "\n";
                        DBG(std::cout << std::string(concrete_intend, ' ') << li.text)
                        DBG(    << li.indent << "," << global_indent << "; l=" << li.nest_level << " i=" << intend << " c=" << intend_correction)
                        DBG(    << "\n";)
                    }
                    DBG(else {)
                    DBG(    std::cout << std::string(concrete_intend, ' ') << "IGNORED")
                    DBG(        << li.indent << "," << global_indent << "; l=" << li.nest_level << " i=" << intend << " c=" << intend_correction)
                    DBG(        << "\n";)
                    DBG(})
                    prev_level = li.nest_level;
                }
                std::string l = out.str();
                l = l.substr(0,l.size()-1); // remove last newline
                return l;
            }
            size_t current_col() { return line.str().size(); }
        };

        inline std::string add_intend_after_newline(std::string input, size_t intend) {
            std::istringstream input_stream{input};
            std::ostringstream output_stream;
            std::string l;
            bool first = true;
            while(std::getline(input_stream, l)) {
                if (!first) {
                    output_stream << std::string(intend, ' ');
                }
                output_stream << l << "\n";
                first = false;
            }
            l = output_stream.str();
            return l.substr(0,l.size()-1);
        }
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
                s << obj->fqn_attributes( (*cmd)["fqn"].str() ).str();
            }
            void format_CommandForLoop(std::shared_ptr<textx::object::Object> cmd) {
                auto name = (*cmd)["name"].str();
                auto obj = get_obj( (*cmd)["obj"].obj() );
                auto fqn_query = (*cmd)["fqn"].str();
                s.inc_level();
                for(auto &e: obj->fqn_attributes(fqn_query)) {
                    s.nontextual_cmd(); // for
                    loop_obj[name] = e.obj();
                    format_obj( (*cmd)["body"]["body"].obj() );
                    s.nontextual_cmd(); // endfor
                }
                s.dec_level();
                loop_obj.erase(name);
            }
            void format_CommandObj2StrFun(std::shared_ptr<textx::object::Object> cmd) {
                auto fun = std::get<textx::istrings::Obj2StrFun>(external_links[(*cmd)["call"]["name"].str()]);
                auto obj = get_obj( (*cmd)["obj"].obj() );
                if (!(*cmd)["fqn"].is_null()) {
                    obj = obj->fqn_attributes( (*cmd)["fqn"]["value"].str() ).obj();
                }
                std::string res = fun(obj);
                res = add_intend_after_newline(res, s.current_col());
                s << res;
            }
            void format_cmd(std::shared_ptr<textx::object::Object> cmd) {
                if (cmd->type=="CommandObjAttributeAsString") format_CommandObjAttributeAsString(cmd);
                else if (cmd->type=="CommandForLoop") format_CommandForLoop(cmd);
                else if (cmd->type=="CommandObj2StrFun") format_CommandObj2StrFun(cmd);
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
}
