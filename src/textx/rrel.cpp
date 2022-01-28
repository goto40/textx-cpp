#include "textx/rrel.h"
#include "textx/assert.h"
#include "textx/arpeggio.h"
#include "textx/lang.h"
#include "textx/metamodel.h"
#include <unordered_map>

#define MYDBG(x)
#define MYYIELD(x) {auto _internal_copy_res = x; co_yield _internal_copy_res;}
//In some cases this seems problematic (see valgrid memory violations...) UNCLEAR!
//#define MYYIELD(x) co_yield x;

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
        else if (choice.children[0].name_is("rule://rrel_dots")) {
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
        std::string fixed_name = "";
        if (m.children[0].children.size()>0) {
            TEXTX_ASSERT_EQUAL(m.children[0].children[0].children.size(), 2);
            consume_name = false;
            TEXTX_ASSERT(m.children[0].children[0].children[0].captured.has_value());
            fixed_name = m.children[0].children[0].children[0].captured.value();
            if (fixed_name.size() > 0) {
                TEXTX_ASSERT(fixed_name.size() > 2)
                fixed_name = fixed_name.substr(1,fixed_name.size()-2);
            }
        }
        std::string name = m.children[1].captured.value();
        return std::make_unique<tr::RRELNavigation>(name, fixed_name, consume_name);
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
        bool use_multimodel = false;
        std::string flags = "";
        if (m.children[0].children.size()>0) {
            flags = m.children[0].children[0].captured.value();
            if (flags.find('p')!=-1) { use_proxy = true; }
            if (flags.find('m')!=-1) { use_multimodel = true; }
        }
        auto ret = std::make_unique<tr::RRELExpression>(
            rrel_sequence(m.children[1]), use_proxy, use_multimodel, flags
        );
        ret->setExpression(ret.get());
        return ret;
    }
}

namespace textx::rrel {
    std::unique_ptr<RRELExpression> create_RREL_expression(const textx::arpeggio::Match& m) {
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

    // misc:
    bool RRELSequence::start_locally() const { 
        bool res = false;
        for(auto &p: paths) {
            res = res || p->start_locally();
        }
        return res;
    }
    bool RRELSequence::start_at_root() const {
        bool res = false;
        for(auto &p: paths) {
            res = res || p->start_at_root();
        }
        return res;
    }
    bool RRELBrackets::start_locally() const { return seq->start_locally(); }
    bool RRELBrackets::start_at_root() const { return seq->start_at_root(); }

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
        if (fixed_name.size()>0) {
            o << "'" << fixed_name << "'";
        }
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
        //std::cout << "Parent(" << type << ")\n";
        auto obj = data.obj;
        auto mm = data.mm;
        obj = obj->parent();
        while(obj!=nullptr) {
            //std::cout << "...Parent(" << obj->type << ")\n";
            if (mm->is_instance(obj->type, type)) break;
            obj = obj->parent();
        }
        if (obj!=nullptr) {
            MYYIELD((py::RRELInternalResult{py::RRELInternalResultData{data.mm, obj,data.lookup_list,data.matched_path}}));
        }
        co_return;
    }

    rrel_generator<const py::RRELInternalResult> RRELBrackets::get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element
    ) const {
        if (allowed(data.obj, data.lookup_list, this)) {
            auto generator_results = seq->get_next_matches(data, allowed, first_element); 
            for (const auto& res: generator_results) {
                MYYIELD(res);
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
                auto generator_results = p->get_next_matches(data, allowed, first_element);
                for (const auto& res: generator_results) {
                    MYYIELD(res);
                }
            }
        }
    }

    rrel_generator<const py::RRELInternalResult> RRELNavigation::get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element
    ) const {
        if (allowed(data.obj, data.lookup_list, this)) {
            if (first_element) { // always start_at_root
                data.obj = data.obj->tx_model()->val().obj();
            }
            std::vector<std::shared_ptr<textx::object::Object>> start;
            start.push_back(data.obj);
            if (data.obj->parent()==nullptr) {
                // not implemented this way in python:
                if (expression->use_multimodel) {
                    for (auto wm: data.obj->tx_model()->tx_imported_models()) {
                        auto m = wm.lock();
                        if (m->val().is_obj()) {
                            auto root = m->val().obj();
                            start.push_back(root);
                        }
                    }
                }
            }
            MYDBG(std::cout << "---- NAVIGATION ----\n";)

            if (data.lookup_list.size()==0 and this->consume_name) {
                co_return;
            }
            MYDBG(if (data.lookup_list.size()>0) std::cout << "lookup:" << data.lookup_list[0] << "\n";)
            
            for (auto obj: start) {
                py::RRELInternalResultData idata = data;
                idata.obj = obj;
                if (idata.obj != nullptr && idata.obj->has_attr(this->name)) {
                    MYDBG(std::cout << "name found...\n";)
                    auto &target = (*idata.obj)[this->name];
                    if (target.is_list()) {
                        MYDBG(std::cout << "is list... #"<< target.size() << "\n";)
                        for (auto& itarget: target) {
                            if (itarget.is_ref() && !itarget.is_resolved()) {
                                MYDBG(std::cout << "postponed...\n";)
                                MYYIELD((textx::scoping::Postponed{}));
                                co_return;
                            }
                            else if (!consume_name) {
                                if (fixed_name.size()>0 && itarget.obj()->has_attr("name")) {
                                    if (itarget["name"].str() == fixed_name) {
                                        MYDBG(std::cout << "no consume with fixed name...\n";)
                                        //std::cout << "no consume with fixed name " << fixed_name << "...\n";
                                        MYYIELD((py::RRELInternalResultData{ data.mm, itarget.obj(), data.lookup_list, data.matched_path }));
                                    }
                                }
                                else {
                                    TEXTX_ASSERT(fixed_name.size()==0, "when specifying a fixed name you need to reference an attribute with a name");
                                    MYDBG(std::cout << "no consume...\n";)
                                    MYYIELD((py::RRELInternalResultData{ data.mm, itarget.obj(), data.lookup_list, data.matched_path }));
                                }
                            }
                            else if (!itarget.is_null() && itarget.obj()->has_attr("name") && itarget["name"].str() == data.lookup_list[0]) {
                                MYDBG(std::cout << "consume...\n";)
                                TEXTX_ASSERT(data.lookup_list.size()>0);
                                std::vector<std::string> lookup_copy{
                                    data.lookup_list.begin()+1,
                                    data.lookup_list.end()
                                };
                                auto matched_path_copy = data.matched_path;
                                matched_path_copy.push_back( itarget.obj() );
                                MYDBG(std::cout << "yield..."<<itarget.obj().get()<<"\n";)
                                MYYIELD((py::RRELInternalResultData{data.mm, itarget.obj(), lookup_copy, matched_path_copy}));
                            }
                        }
                    }
                    else { // scalar
                        //std::cout << "is scalar...\n";
                        auto &itarget = target;
                        if (itarget.is_ref() && !itarget.is_resolved()) {
                            MYYIELD((textx::scoping::Postponed{}));
                            co_return;
                        }
                        else if (!consume_name) {
                            MYYIELD((py::RRELInternalResultData{ data.mm, itarget.obj(), data.lookup_list, data.matched_path }));
                        }
                        else if (!itarget.is_null() && itarget.obj()->has_attr("name") && itarget["name"].str() == data.lookup_list[0]) {
                            std::vector<std::string> lookup_copy{
                                data.lookup_list.begin()+1,
                                data.lookup_list.end()
                            };
                            auto matched_path_copy = data.matched_path;
                            matched_path_copy.push_back( itarget.obj() );
                            MYYIELD((py::RRELInternalResultData{ data.mm, itarget.obj(), lookup_copy, matched_path_copy}));
                        }
                    }
                }
            }
        }
        co_return;
    }

    rrel_generator<const py::RRELInternalResult> RRELDots::get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element
    ) const {
        size_t counter = n;
        auto obj = data.obj;
        while(counter>1 && obj->parent()!=nullptr) {
            obj = obj->parent();
            counter--;
        }
        if (counter==1) {
            MYYIELD((py::RRELInternalResult{py::RRELInternalResultData{ data.mm, obj, data.lookup_list, data.matched_path}}));
        }
        co_return;
    }

    rrel_generator<const py::RRELInternalResult> RRELZeroOrMore::intern_get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element,
        std::unordered_set<std::pair<const textx::object::Object*,size_t>,textx::utils::pair_hash> prevent_doubles
    ) const {
        TEXTX_ASSERT(start_locally() || start_at_root()); 
        if (allowed(data.obj, data.lookup_list, this)) {
            if (first_element) {
                if (start_locally()) {
                    MYYIELD(data);
                }
                if (start_at_root()) {
                    if (data.obj->tx_model()->val().is_obj()) {
                        auto root = data.obj->tx_model()->val().obj();
                        MYYIELD((py::RRELInternalResult{py::RRELInternalResultData{data.mm,root,data.lookup_list,data.matched_path}}));
                    }
                }
            }
            else {
                MYYIELD(data);
            }
            //TODO? TEXTX_ASSERT( textx::utils::is_instance<RRELSequence>(*path_element) );
            auto generator_results = path_element->get_next_matches(data, allowed, first_element);
            for (const auto& res: generator_results) {
                if( std::holds_alternative<textx::scoping::Postponed>(res)) {
                    MYYIELD((textx::scoping::Postponed{}));
                    co_return;
                }
                auto igenerator_results = intern_get_next_matches(std::get<0>(res), allowed, false, prevent_doubles);
                for (const auto& ires: igenerator_results) {
                    MYYIELD(ires);
                }
            }
        }
        co_return;
    }

    rrel_generator<const py::RRELInternalResult> RRELZeroOrMore::get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element
    ) const {
        std::unordered_set<std::pair<const textx::object::Object*,size_t>,textx::utils::pair_hash> prevent_doubles{};
        auto igenerator_results = intern_get_next_matches(data, allowed, first_element, prevent_doubles);
        for (const auto& res: igenerator_results) {
            if( std::holds_alternative<textx::scoping::Postponed>(res)) {
                MYYIELD((textx::scoping::Postponed{}));
                co_return;
            }
            auto &d = std::get<0>(res);
            auto key = std::make_pair(d.obj.get(),d.lookup_list.size());
            if (prevent_doubles.count(key)==0) {
                prevent_doubles.insert(key);
                MYYIELD(res);
            }
        }
        co_return;
    }

    rrel_generator<const py::RRELInternalResult> RRELPath::intern_get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element,
        size_t idx
    ) const {
        TEXTX_ASSERT(path_elements.size() > idx);
        auto &e = path_elements[idx];
        auto generator_results = e->get_next_matches(data, allowed, first_element);
        for (const auto& res: generator_results) {
            if( std::holds_alternative<textx::scoping::Postponed>(res)) {
                MYYIELD((textx::scoping::Postponed{}));
                co_return;
            }
            if (path_elements.size() - 1 == idx) {
                MYYIELD(res);
            }
            else {
                if (std::get<0>(res).obj == nullptr) {
                    co_return;
                }
                auto igenerator_results = intern_get_next_matches(std::get<0>(res), allowed, false,idx+1);
                for (const auto& ires: igenerator_results) {
                    MYYIELD(ires);
                }
            }
        }
    }

    rrel_generator<const py::RRELInternalResult> RRELPath::get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element
    ) const {
        auto igenerator_results = intern_get_next_matches(data, allowed, first_element);
        for (const auto& res: igenerator_results) {
            MYYIELD(res);
        }
    }

    rrel_generator<const py::RRELInternalResult> RRELExpression::get_next_matches(
        py::RRELInternalResultData data,
        AllowedFunc allowed,
        bool first_element
    ) const {
        TEXTX_ASSERT(allowed(data.obj, data.lookup_list, this));
        auto generator_results = seq->get_next_matches(data, allowed, first_element);
        for (const auto& res: generator_results) {
            MYYIELD(res);
        }
    }

    py::RRELResult find_object_with_path(std::shared_ptr<textx::object::Object> obj, std::vector<std::string> lookup, textx::rrel::RRELExpression& rrel_tree, std::string obj_cls)
    {
        auto allowed = [
            visited=std::vector<std::unordered_set<std::pair<const textx::object::Object*, const textx::rrel::RRELBase*>,textx::utils::pair_hash>>(lookup.size()+1) // recursion breaker
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
        auto generator_results = rrel_tree.get_next_matches({obj->tx_model()->tx_metamodel(), obj, lookup, {}}, std::ref(allowed), "");
        for (const py::RRELInternalResult res : generator_results) {
            if(std::holds_alternative<textx::scoping::Postponed>(res)) {
                MYDBG(std::cout << "FINAL: POSTPONED\n";)
                return textx::scoping::Postponed{};
            }
            else if (std::get<0>(res).lookup_list.size()==0 && std::get<0>(res).obj!=nullptr) {
                auto obj = std::get<0>(res).obj;
                auto mm = std::get<0>(res).mm;
                if (obj_cls=="" || mm->is_instance(obj->type, obj_cls)) {
                    MYDBG(std::cout << "FINAL: RES, ";)
                    MYDBG(obj->print(std::cout);)
                    MYDBG(std::cout <<"\n";)
                    return py::RRELResultData{obj, std::get<0>(res).matched_path};
                }
            }
        }
        return py::RRELResultData{nullptr,{}};
    }

    std::tuple<std::shared_ptr<textx::object::Object>, MatchedPath> RRELScopeProvider::resolve(std::shared_ptr<textx::object::Object> origin, std::string obj_name, std::optional<std::string> target_type) const
    {
        std::string type="";
        if (target_type.has_value()) {
            type = *target_type;
        }
        auto res = textx::rrel::find_object_with_path(
            origin,
            obj_name,
            *rrel_expression,
            type,
            split_string);
        if (std::holds_alternative<textx::scoping::Postponed>(res)) {
            return {nullptr, {}};
        }
        else {
            auto [obj, objpath] = std::get<0>(res);
            return {obj, objpath};
        }
    }

    void RRELExpression::setExpression(RRELExpression *e) { TEXTX_ASSERT(e==this); seq->setExpression(e); }
    void RRELBrackets::setExpression(RRELExpression *e) { expression=e; seq->setExpression(e); }
    void RRELSequence::setExpression(RRELExpression *e) { expression=e; for (auto &p: paths) p->setExpression(e); }
    void RRELPath::setExpression(RRELExpression *e) { expression=e; for (auto &p: path_elements) p->setExpression(e); }

}