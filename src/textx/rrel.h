#pragma once

//#define USE_CPP_CORO

#include "textx/arpeggio.h"
#include "textx/object.h"
#include "textx/scoping.h"
#include "textx/utils.h"
#ifdef USE_CPP_CORO
#include <cppcoro/generator.hpp>
#endif
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <sstream>

namespace textx::rrel {

#ifdef USE_CPP_CORO
    template<class T> using rrel_generator = cppcoro::generator<T>;
#else
    template<class T> using rrel_generator = textx::utils::Generator<T>;
#endif

    using MatchedPath = textx::scoping::MatchedPath;
    namespace py {
        struct RRELInternalResultData {
            std::shared_ptr<textx::Metamodel> mm;
            std::shared_ptr<textx::object::Object> obj;
            std::vector<std::string> lookup_list;
            MatchedPath matched_path;
        };
        using RRELInternalResult = std::variant<
            RRELInternalResultData,
            textx::scoping::Postponed
        >;
        struct RRELResultData {
            std::shared_ptr<textx::object::Object> obj;
            MatchedPath matched_path;
        };
        using RRELResult = std::variant<
            RRELResultData,
            textx::scoping::Postponed
        >;
    }

    struct RRELBase;
    using AllowedFunc = std::function<bool(std::shared_ptr<textx::object::Object>, std::vector<std::string>, const RRELBase*)>;

    struct RRELExpression;
    struct RRELBase {
        virtual ~RRELBase() = default;
        virtual void print(std::ostream& o) const=0;
        virtual void setExpression(RRELExpression *e)=0;
        virtual rrel_generator<const py::RRELInternalResult> get_next_matches(
            py::RRELInternalResultData data,
            AllowedFunc allowed,
            bool first_element=false
        ) const =0;
        virtual bool start_locally() const { return false; }
        virtual bool start_at_root() const { return false; }
        std::string str() {
            std::ostringstream o;
            print(o);
            return o.str();
        }
    };

    struct RRELPathElement : RRELBase {
    };

    struct RRELSequence;

    struct RRELExpression : RRELBase {
        std::unique_ptr<RRELSequence> seq;
        std::string flags;
        bool use_proxy;
        bool use_multimodel;
        // note: importURI handled differently (ignored here)
        RRELExpression(std::unique_ptr<RRELSequence> &&seq, bool use_proxy, bool use_multimodel, std::string flags) : seq{std::move(seq)}, use_proxy{use_proxy}, use_multimodel{use_multimodel}, flags{flags} {}
        void print(std::ostream& o) const override;
        void setExpression(RRELExpression *e) override;
        rrel_generator<const py::RRELInternalResult> get_next_matches(
            py::RRELInternalResultData data,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
    };

    struct RRELParent : RRELPathElement {
        std::string type;
        RRELExpression *expression;
        void setExpression(RRELExpression *e) override { expression=e; }
        RRELParent(std::string type) : type{std::move(type)} {}
        void print(std::ostream& o) const override;
        rrel_generator<const py::RRELInternalResult> get_next_matches(
            py::RRELInternalResultData data,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
        bool start_locally() const override { return true; }
    };

    struct RRELNavigation : RRELPathElement {
        std::string name;
        std::string fixed_name;
        bool consume_name;
        RRELExpression *expression;
        void setExpression(RRELExpression *e) override { expression=e; }
        RRELNavigation(std::string name, std::string fixed_name, bool consume_name) : name{std::move(name)}, fixed_name{fixed_name}, consume_name{consume_name} {}
        void print(std::ostream& o) const override;
        rrel_generator<const py::RRELInternalResult> get_next_matches(
            py::RRELInternalResultData data,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
        bool start_at_root() const override { return true; }
    };

    struct RRELSequence;
    struct RRELBrackets : RRELPathElement {
        std::unique_ptr<RRELSequence> seq;
        RRELBrackets(std::unique_ptr<RRELSequence> &&seq) : seq{std::move(seq)} {}
        RRELExpression *expression;
        void setExpression(RRELExpression *e) override;
        void print(std::ostream& o) const override;
        rrel_generator<const py::RRELInternalResult> get_next_matches(
            py::RRELInternalResultData data,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
        bool start_locally() const override;
        bool start_at_root() const override;
        
    };

    struct RRELDots : RRELPathElement {
        size_t n;
        RRELDots(size_t n) : n{n} {}
        void print(std::ostream& o) const override;
        RRELExpression *expression;
        void setExpression(RRELExpression *e) override { expression=e; }
        rrel_generator<const py::RRELInternalResult> get_next_matches(
            py::RRELInternalResultData data,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
        bool start_locally() const override { return true; }
    };

    struct RRELPath;
    struct RRELSequence : RRELBase {
        std::vector<std::unique_ptr<RRELPath>> paths;
        RRELSequence(std::vector<std::unique_ptr<RRELPath>> &&paths) : paths{std::move(paths)} {};
        void print(std::ostream& o) const override;
        RRELExpression *expression;
        void setExpression(RRELExpression *e) override;
        rrel_generator<const py::RRELInternalResult> get_next_matches(
            py::RRELInternalResultData data,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
        bool start_locally() const override;
        bool start_at_root() const override;
    };

    struct RRELZeroOrMore : RRELPathElement {
        std::unique_ptr<RRELPathElement> path_element;
        RRELZeroOrMore(std::unique_ptr<RRELPathElement> path_element) : path_element{std::move(path_element)} {}
        void print(std::ostream& o) const override;
        RRELExpression *expression;
        void setExpression(RRELExpression *e) override { expression=e; path_element->setExpression(e); }
        rrel_generator<const py::RRELInternalResult> get_next_matches(
            py::RRELInternalResultData data,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
        bool start_locally() const override { return path_element->start_locally(); }
        bool start_at_root() const override { return path_element->start_at_root(); }
        private:
        rrel_generator<const py::RRELInternalResult> intern_get_next_matches(
            py::RRELInternalResultData data,
            AllowedFunc allowed,
            bool first_element,
            std::unordered_set<std::pair<const textx::object::Object*,size_t>,textx::utils::pair_hash> prevent_doubles
        ) const;
    };

    struct RRELPath : RRELBase {
        std::vector<std::unique_ptr<RRELPathElement>> path_elements;
        RRELPath(std::vector<std::unique_ptr<RRELPathElement>> &&path_elements) : path_elements{std::move(path_elements)} {};       
        void print(std::ostream& o) const override;
        RRELExpression *expression;
        void setExpression(RRELExpression *e) override;
        rrel_generator<const py::RRELInternalResult> get_next_matches(
            py::RRELInternalResultData data,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
        bool start_locally() const override {
            if (path_elements.size()==0) return false;
            else return path_elements[0]->start_locally();
        }
        bool start_at_root() const override {
            if (path_elements.size()==0) return false;
            else return path_elements[0]->start_at_root();
        }
        private:
        rrel_generator<const py::RRELInternalResult> intern_get_next_matches(
            py::RRELInternalResultData data,
            AllowedFunc allowed,
            bool first_element=false,
            size_t idx=0
        ) const;
    };

    std::unique_ptr<RRELExpression> create_RREL_expression(std::shared_ptr<const textx::arpeggio::Match> m);
    std::unique_ptr<RRELExpression> create_RREL_expression(std::string rrel_expression_string);

    py::RRELResult find_object_with_path(std::shared_ptr<textx::object::Object> obj, std::vector<std::string> lookup, textx::rrel::RRELExpression& rrel_tree, std::string obj_cls="");
    inline py::RRELResult find_object_with_path(
        std::shared_ptr<textx::object::Object> obj,
        std::variant<std::string,std::vector<std::string>> lookup,
        textx::rrel::RRELExpression& rrel_tree, 
        std::string obj_cls="", 
        std::string split_string=".")
    {
        std::vector<std::string> new_lookup {};
        if (std::holds_alternative<std::string>(lookup)) {
            std::string& str = std::get<std::string>(lookup);
            size_t start;
            size_t end = 0;
        	while ((start = str.find_first_not_of(split_string, end)) != std::string::npos)
            {
                end = str.find(split_string, start);
                new_lookup.push_back(str.substr(start, end - start));
            }
        }
        else {
            new_lookup = std::move(std::get<std::vector<std::string>>(lookup));
        }
        return find_object_with_path(obj, new_lookup, rrel_tree, obj_cls);
    }
    inline py::RRELResult find_object_with_path(
        std::shared_ptr<textx::object::Object> obj,
        std::variant<std::string,std::vector<std::string>> lookup,
        std::string rrel_tree, 
        std::string obj_cls="", 
        std::string split_string=".") 
    {
        auto rrel = create_RREL_expression(rrel_tree);
        return find_object_with_path(obj, lookup, *rrel, obj_cls, split_string);
    }

    /** note: return nullptr in case of postponed elements */
    inline std::shared_ptr<textx::object::Object> find(
        std::shared_ptr<textx::object::Object> obj,
        std::string lookup,
        std::string rrel_tree,
        std::string obj_cls="",
        std::string split_string=".") 
    {
        auto res = find_object_with_path(obj, lookup, rrel_tree, obj_cls, split_string);
        if(std::holds_alternative<textx::scoping::Postponed>(res)) {
            return nullptr;
        }
        else {
            return std::get<0>(res).obj;
        }
    }

    inline std::string build_fqn(const MatchedPath& objpath, std::string separator=".") {
        std::string n="";
        for (size_t i=0;i<objpath.size();i++) {
            if (i>0) { n = n + separator; }
            auto pe = objpath[i].lock();
            TEXTX_ASSERT(pe != nullptr);
            n = n+(*pe)["name"].str();
        }
        return n;
    }

    class RRELScopeProvider : public textx::scoping::RefResolver {
        std::unique_ptr<RRELExpression> rrel_expression;
        std::string split_string;
    public:
        RRELScopeProvider(std::shared_ptr<const textx::arpeggio::Match> m,std::string split_string=".") : rrel_expression{create_RREL_expression(m)}, split_string{split_string} {}
        RRELScopeProvider(std::string rrel_string,std::string split_string=".") : rrel_expression{create_RREL_expression(rrel_string)}, split_string{split_string} {}
        std::tuple<std::shared_ptr<textx::object::Object>, MatchedPath> resolve(std::shared_ptr<textx::object::Object> origin, std::string obj_name, std::optional<std::string> target_type) const override;
    };
}
