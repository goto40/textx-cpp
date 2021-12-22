#include "textx/rrel.h"
#include "textx/assert.h"
#include "textx/arpeggio.h"
#include "textx/lang.h"
#include <unordered_map>

#define MYDBG(x)
namespace {
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator () (const std::pair<T1,T2> &p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ h2;  
        }
    };    
}

namespace ta=textx::arpeggio;
namespace tr=textx::rrel;
namespace {
    std::unique_ptr<tr::RRELPathElement> rrel_path_element(const ta::Match& choice);
    std::unique_ptr<tr::RRELSequence> rrel_sequence(const ta::Match& m);

    std::unique_ptr<tr::RRELZeroOrMore> create_circumflex() {
        std::vector<std::unique_ptr<tr::RRELPathElement>> path_elements{};
        path_elements.push_back(std::make_unique<tr::RRELDots>(2));
        std::vector<std::unique_ptr<tr::RRELPath>> paths{};
        paths.push_back(std::make_unique<tr::RRELPath>(std::move(path_elements)));
        return std::make_unique<tr::RRELZeroOrMore>(
            std::make_unique<tr::RRELBrackets>(
                std::make_unique<tr::RRELSequence>(
                    std::move(paths)
                )
            )
        );
    }

    void process_start_of_path(const ta::Match& choice, std::vector<std::unique_ptr<tr::RRELPathElement>> &path_elements) {
        TEXTX_ASSERT(choice.type() == ta::MatchType::ordered_choice, "unexpected (process_start_of_path) ");
        TEXTX_ASSERT(choice.children.size()==1, " unexpected, expected choice (process_start_of_path) ", choice);
        if (choice.children[0].type()==ta::MatchType::str_match && choice.children[0].captured.value() == "^") {
            path_elements.push_back(create_circumflex());
        }
        else if (choice.children[0].name_is("textx://rrel_dots")) {
            path_elements.push_back(std::make_unique<tr::RRELDots>(choice.children[0].captured.value().size()));
        }
        else {
            ta::raise(choice.children[0].start(), "unexpected match found (process_start_of_path) ", choice.children[0]);
        }
    }

    std::unique_ptr<tr::RRELZeroOrMore> rrel_zero_or_more(const ta::Match& m) {
        TEXTX_ASSERT(m.name_is("rule://rrel_zero_or_more"), " unexpected: ", m);
        TEXTX_ASSERT(m.children.size()==2, " unexpected, expected seq with '*' at the end ", m);
        if (m.children[0].name_is("rule://rrel_brackets")) {
            return std::make_unique<tr::RRELZeroOrMore>(rrel_path_element(m.children[0]));
        }
        else if (m.children[0].name_is("rule://rrel_path_element") && m.children[0].children[0].name_is("rule://rrel_brackets")) {
            return std::make_unique<tr::RRELZeroOrMore>(rrel_path_element(m.children[0]));
        }
        else {
            std::vector<std::unique_ptr<tr::RRELPath>> paths{};
            std::vector<std::unique_ptr<tr::RRELPathElement>> path_elements{};
            path_elements.push_back(rrel_path_element(m.children[0]));
            paths.push_back(std::make_unique<tr::RRELPath>( std::move(path_elements)));
            return std::make_unique<tr::RRELZeroOrMore>(
                std::make_unique<tr::RRELBrackets>(
                    std::make_unique<tr::RRELSequence>( std::move(paths) )
                )
            );
        }
    }

    std::unique_ptr<tr::RRELParent> rrel_parent(const ta::Match& m) {
        TEXTX_ASSERT(m.name_is("rule://rrel_parent"), " unexpected: ", m);
        TEXTX_ASSERT(m.children.size()==4, " unexpected, expected parent(type) ", m);
        std::string type = m.children[2].captured.value();
        return std::make_unique<tr::RRELParent>(type);
    }

    std::unique_ptr<tr::RRELBrackets> rrel_brackets(const ta::Match& m) {
        TEXTX_ASSERT(m.name_is("rule://rrel_brackets"), " unexpected: ", m);
        TEXTX_ASSERT(m.children.size()==3, " unexpected, expected '(' seq. ')' ", m);
        return std::make_unique<tr::RRELBrackets>(rrel_sequence(m.children[1]));
    }

    std::unique_ptr<tr::RRELNavigation> rrel_navigation(const ta::Match& m) {
        TEXTX_ASSERT(m.name_is("rule://rrel_navigation"), " unexpected: ", m);
        TEXTX_ASSERT(m.children.size()==2, " unexpected, expected seq with '*' at the end ", m);
        bool consume_name = true;
        if (m.children[0].captured.value() == "~") {
            consume_name = false;
        }
        std::string name = m.children[1].captured.value();
        return std::make_unique<tr::RRELNavigation>(name, consume_name);
    }

    std::unique_ptr<tr::RRELPathElement> rrel_path_element(const ta::Match& choice) {
        TEXTX_ASSERT(choice.type() == ta::MatchType::ordered_choice);
        TEXTX_ASSERT_EQUAL(choice.children.size(), 1);
        if (choice.children[0].name_is("rule://rrel_zero_or_more")) {
            return rrel_zero_or_more(choice.children[0]);
        }
        else if (choice.children[0].name_is("rule://rrel_parent")) {
            return rrel_parent(choice.children[0]);
        }
        else if (choice.children[0].name_is("rule://rrel_brackets")) {
            return rrel_brackets(choice.children[0]);
        }
        else if (choice.children[0].name_is("rule://rrel_navigation")) {
            return rrel_navigation(choice.children[0]);
        }
        else if (choice.children[0].name_is("rule://rrel_path_element")) {
            return rrel_path_element(choice.children[0]);
        }
        else {
            ta::raise(choice.children[0].start(), " unexpected match ", choice.children[0], " in ", choice);
        }
    }

    std::unique_ptr<tr::RRELPath> rrel_path(const ta::Match& m) {
        std::vector<std::unique_ptr<tr::RRELPathElement>> path_elements;
        TEXTX_ASSERT(m.name_is("rule://rrel_path"), "unexpected: ", m);
        TEXTX_ASSERT(m.children.size()==1, " unexpected, expected choice ", m);
        if (m.children[0].type() == ta::MatchType::sequence) { // #1
            const auto &seq = m.children[0];
            // start
            if (seq.children[0].children.size()>0) process_start_of_path(seq.children[0].children[0], path_elements);
            // zero or more
            for (const auto& im: seq.children[1].children) {
                path_elements.push_back(rrel_path_element(im.children[0]));
            }
            // final path entry
            path_elements.push_back(rrel_path_element(seq.children[2]));
        }
        else { // #2
            const auto &choice = m.children[0];
            process_start_of_path(choice, path_elements);
        }
        return std::make_unique<tr::RRELPath>(std::move(path_elements));
    }
    std::unique_ptr<tr::RRELSequence> rrel_sequence(const ta::Match& m) {
        TEXTX_ASSERT(m.name_is("rule://rrel_sequence"), "unexpected: ", m);
        std::vector<std::unique_ptr<tr::RRELPath>> v;
        for(const auto& im: m.children[0].children) {
            TEXTX_ASSERT(im.children.size()==2, " unexpected, expected sequence with two entries and a rrel_path at first pos ", im);
            v.push_back( rrel_path(im.children[0]) );
        }
        v.push_back( rrel_path(m.children[1]) );
        return std::make_unique<tr::RRELSequence>(std::move(v));
    }
    std::unique_ptr<tr::RRELExpression> rrel_expression(const ta::Match& m) {
        TEXTX_ASSERT(m.name_is("rule://rrel_expression"), "unexpected: ", m);
        TEXTX_ASSERT(m.children.size()==2," unexpected ",m);
        bool use_proxy = false;
        std::string flags = "";
        if (m.children[0].children.size()>0) {
            flags = m.children[0].children[0].captured.value();
            if (flags.find('p')!=-1) { use_proxy = true; }
        }
        return std::make_unique<tr::RRELExpression>(
            rrel_sequence(m.children[1]), use_proxy, flags
        );
    }
}

namespace textx::rrel {
    std::unique_ptr<RRELExpression> create_RREL_expression(textx::arpeggio::Match m) {
        return rrel_expression(m);
    }
    std::unique_ptr<RRELExpression> create_RREL_expression(std::string rrel_expression_string) {
        textx::lang::TextxGrammar parser;
        parser.add_rule("rrel_expression_standalone", 
                        textx::arpeggio::sequence({
                            parser.ref("rrel_expression"),
                            textx::arpeggio::end_of_file()
                        }));
        parser.set_main_rule("rrel_expression_standalone");
        return textx::rrel::create_RREL_expression(parser.parse_or_throw(rrel_expression_string)->children[0]);
    }

    // print:

    void RRELParent::print(std::ostream& o) const {
        o << "parent(" << type << ")";
    }
    void RRELBrackets::print(std::ostream& o) const {
        o << "(";
        seq->print(o);
        o << ")";
    }
    void RRELSequence::print(std::ostream& o) const {
        bool first=true;
        for(auto& p: paths) {
            if (!first) { o << ","; }
            p->print(o);
            first = false;
        }
    }
    void RRELNavigation::print(std::ostream& o) const { 
        if (!consume_name) { o << "~"; }
        o << name;
    }
    void RRELDots::print(std::ostream& o) const {
        o << std::string(n, '.');
    }
    void RRELZeroOrMore::print(std::ostream& o) const {
        path_element->print(o);
        o << "*";
    }
    void RRELPath::print(std::ostream& o) const {
        bool first=true;
        for(auto& e: path_elements) {
            if (!first) { o << "."; }
            e->print(o);
            first = false;
        }
    }
    void RRELExpression::print(std::ostream& o) const {
        o << flags;
        seq->print(o);
    }

    // resolve:
    rrel_generator<const py::RRELInternalResult> RRELParent::get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element
    ) const {
        co_yield textx::scoping::Postponed{}; // TODO
    }

    rrel_generator<const py::RRELInternalResult> RRELBrackets::get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element
    ) const {
        if (allowed(data.obj, data.lookup_list, this)) {
            for (const auto& res: seq->get_next_matches(data, allowed, first_element)) {
                co_yield res;
            }
        }
    }

    rrel_generator<const py::RRELInternalResult> RRELSequence::get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element
    ) const {
        if (allowed(data.obj, data.lookup_list, this)) {
            for (const auto& p: paths) {
                for (const auto& res: p->get_next_matches(data, allowed, first_element)) {
                    co_yield res;
                }
            }
        }
    }

    rrel_generator<const py::RRELInternalResult> RRELNavigation::get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element
    ) const {
        if (first_element) {
            data.obj = data.obj->tx_model()->val().obj();
        }
        MYDBG(std::cout << "---- NAVIGATION ----\n";)

        if (data.lookup_list.size()==0 and this->consume_name) {
            auto res = py::RRELInternalResultData{nullptr, data.lookup_list, data.matched_path};
            co_yield res;
            //co_yield py::RRELInternalResultData{nullptr, data.lookup_list, data.matched_path};
            co_return;
        }
        MYDBG(std::cout << "lookup:" << data.lookup_list[0] << "\n";)
        
        if (data.obj == nullptr) {
            MYDBG(std::cout << "is null...\n";)
            auto res = py::RRELInternalResultData{nullptr, data.lookup_list, data.matched_path};
            co_yield res;
            // co_yield py::RRELInternalResultData{nullptr, data.lookup_list, data.matched_path};
            co_return;
        }
        else if (data.obj->has_attr(this->name)) {
            MYDBG(std::cout << "name found...\n";)
            auto &target = (*data.obj)[this->name];
            if (target.is_list()) {
                MYDBG(std::cout << "is list...\n";)
                for (auto& itarget: target) {
                    if (itarget.is_ref() && !itarget.is_resolved()) {
                        MYDBG(std::cout << "postponed...\n";)
                        co_yield textx::scoping::Postponed{};
                        co_return;
                    }
                    else if (!consume_name) {
                        MYDBG(std::cout << "no consume...\n";)
                        auto res = py::RRELInternalResultData{ itarget.obj(), data.lookup_list, data.matched_path };
                        co_yield res;
                        // co_yield py::RRELInternalResultData{ itarget.obj(), data.lookup_list, data.matched_path };
                    }
                    else if (itarget.obj()->has_attr("name") && itarget["name"].str() == data.lookup_list[0]) {
                        MYDBG(std::cout << "consume...\n";)
                        TEXTX_ASSERT(data.lookup_list.size()>0);
                        std::vector<std::string> lookup_copy{
                            data.lookup_list.begin()+1,
                            data.lookup_list.end()
                        };
                        auto matched_path_copy = data.matched_path;
                        matched_path_copy.push_back( itarget.obj() );
                        MYDBG(std::cout << "yield..."<<itarget.obj().get()<<"\n";)
                        auto res = py::RRELInternalResultData{itarget.obj(), lookup_copy, matched_path_copy};
                        co_yield res;
                        // Problem was... (local copy solved the problem...)
                        //co_yield py::RRELInternalResultData{itarget.obj(), lookup_copy, matched_path_copy};
                        //std::cout << "end of consume...\n";
                        co_return;
                    }
                    else {
                        MYDBG(std::cout << "nullptr...\n";)
                        auto res = py::RRELInternalResultData{nullptr, data.lookup_list, data.matched_path};
                        co_yield res;
                        //co_yield py::RRELInternalResultData{nullptr, data.lookup_list, data.matched_path};
                        co_return;
                    }

                }
            }
            else { // scalar
                //std::cout << "is scalar...\n";
                auto &itarget = target;
                if (itarget.is_ref() && !itarget.is_resolved()) {
                    co_yield textx::scoping::Postponed{};
                    co_return;
                }
                else if (!consume_name) {
                    co_yield py::RRELInternalResultData{ itarget.obj(), data.lookup_list, data.matched_path };
                }
                else if (itarget.obj()->has_attr("name") && itarget["name"].str() == data.lookup_list[0]) {
                    std::vector<std::string> lookup_copy{
                        data.lookup_list.begin()+1,
                        data.lookup_list.end()
                    };
                    auto matched_path_copy = data.matched_path;
                    matched_path_copy.push_back( itarget.obj() );
                    auto res = py::RRELInternalResultData{itarget.obj(), lookup_copy, matched_path_copy};
                    co_yield res;
                    //co_yield py::RRELInternalResultData{itarget.obj(), lookup_copy, matched_path_copy};
                    co_return;
                }
                else {
                    auto res = py::RRELInternalResultData{nullptr, data.lookup_list, data.matched_path};
                    co_yield res;
                    //co_yield py::RRELInternalResultData{nullptr, data.lookup_list, data.matched_path};
                    co_return;
                }
            }
        }
        else {
            MYDBG(std::cout << "name "<< name << " not found in "<< data.obj->type <<"...\n";)
            auto res = py::RRELInternalResultData{nullptr, data.lookup_list, data.matched_path};
            co_yield res;
            // co_yield py::RRELInternalResultData{nullptr, data.lookup_list, data.matched_path};
            co_return;
        }
    }

    rrel_generator<const py::RRELInternalResult> RRELDots::get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element
    ) const {
        co_yield textx::scoping::Postponed{}; // TODO
    }

    rrel_generator<const py::RRELInternalResult> RRELZeroOrMore::get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element
    ) const {
TODO
    }

    rrel_generator<const py::RRELInternalResult> RRELPath::intern_get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element,
        size_t idx
    ) const {
        TEXTX_ASSERT(path_elements.size() > idx);
        auto &e = path_elements[idx];
        for (const auto& res: e->get_next_matches(data, allowed, first_element)) {
            if( std::holds_alternative<textx::scoping::Postponed>(res)) {
                co_yield textx::scoping::Postponed{};
                co_return;
            }
            if (path_elements.size() - 1 == idx) {
                co_yield res;
            }
            else {
                if (std::get<0>(res).obj == nullptr) {
                    co_yield py::RRELInternalResultData{nullptr, data.lookup_list, data.matched_path};
                    co_return;
                }
                for (const auto& ires: intern_get_next_matches(std::get<0>(res), allowed, false,idx+1)) {
                    co_yield ires;
                }
            }
        }
    }

    rrel_generator<const py::RRELInternalResult> RRELPath::get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element
    ) const {
        for (const auto& res: intern_get_next_matches(data, allowed, first_element)) {
            co_yield res;
        }
    }

    rrel_generator<const py::RRELInternalResult> RRELExpression::get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element
    ) const {
        TEXTX_ASSERT(allowed(data.obj, data.lookup_list, this));
        for (const auto& res: seq->get_next_matches(data, allowed, first_element)) {
            co_yield res;
        }
    }

    py::RRELResult find_object_with_path(std::shared_ptr<textx::object::Object> obj, std::vector<std::string> lookup, textx::rrel::RRELExpression& rrel_tree, std::string obj_cls)
    {
        auto allowed = [
            visited=std::vector<std::unordered_set<std::pair<textx::object::Object*, const textx::rrel::RRELBase*>,pair_hash>>(lookup.size()+1) // recursion breaker
        ](std::shared_ptr<textx::object::Object> obj, std::vector<std::string> lookup_list, const textx::rrel::RRELBase* e) mutable {
            TEXTX_ASSERT(lookup_list.size()<visited.size())
            std::pair<textx::object::Object*, const textx::rrel::RRELBase*> elem{obj.get(), e};
            if (visited[lookup_list.size()].count(elem)>0) {
                return false;
            }
            else {
                visited[lookup_list.size()].insert(elem);
                return true;
            }
        };
        // important: use ref(...) here to protect (do not duplicate) the state
        for (const py::RRELInternalResult res : rrel_tree.get_next_matches({obj, lookup, {}}, std::ref(allowed), "")) {
            if(std::holds_alternative<textx::scoping::Postponed>(res)) {
                MYDBG(std::cout << "FINAL: POSTPONED\n";)
                return textx::scoping::Postponed{};
            }
            else if (std::get<0>(res).lookup_list.size()==0 && std::get<0>(res).obj!=nullptr) {
                MYDBG(std::cout << "FINAL: RES, ";)
                MYDBG(std::get<0>(res).obj->print(std::cout);)
                MYDBG(std::cout <<"\n";)
                return py::RRELResultData{std::get<0>(res).obj, std::get<0>(res).matched_path};
            }
        }
        return py::RRELResultData{nullptr,{}};
    }

}