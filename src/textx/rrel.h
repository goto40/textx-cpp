#pragma once
#include "textx/arpeggio.h"
#include "textx/object.h"
#include "textx/scoping.h"
#include <cppcoro/generator.hpp>
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <sstream>

namespace textx::rrel {

    using RRELInternalResult = std::variant<
        std::tuple<
            std::shared_ptr<textx::object::Object>,
            std::vector<std::string>,
            std::vector<std::shared_ptr<textx::object::Object>>
        >,
        textx::scoping::Postponed
    >;
    using RRELResultData = std::tuple<
        std::shared_ptr<textx::object::Object>,
        std::vector<std::shared_ptr<textx::object::Object>>
    >;
    using RRELResult = std::variant<
        RRELResultData,
        textx::scoping::Postponed
    >;

    struct RRELBase;
    using AllowedFunc = std::function<bool(std::shared_ptr<textx::object::Object>, std::vector<std::string>, RRELBase*)>;

    struct RRELBase {
        virtual ~RRELBase() = default;
        virtual void print(std::ostream& o) const=0;
        virtual cppcoro::generator<const RRELInternalResult> get_next_matches(
            std::shared_ptr<textx::object::Object>, 
            std::vector<std::string> lookup_list,
            AllowedFunc allowed,
            bool first_element=false
        ) const =0;
        std::string str() {
            std::ostringstream o;
            print(o);
            return o.str();
        }
    };

    struct RRELPathElement : RRELBase {
    };

    struct RRELParent : RRELPathElement {
        std::string type;
        RRELParent(std::string type) : type{std::move(type)} {}
        void print(std::ostream& o) const override;
        cppcoro::generator<const RRELInternalResult> get_next_matches(
            std::shared_ptr<textx::object::Object>, 
            std::vector<std::string> lookup_list,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
    };

    struct RRELNavigation : RRELPathElement {
        std::string name;
        bool consume_name;
        RRELNavigation(std::string name, bool consume_name) : name{std::move(name)}, consume_name{consume_name} {}
        void print(std::ostream& o) const override;
        cppcoro::generator<const RRELInternalResult> get_next_matches(
            std::shared_ptr<textx::object::Object>, 
            std::vector<std::string> lookup_list,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
    };

    struct RRELSequence;
    struct RRELBrackets : RRELPathElement {
        std::unique_ptr<RRELSequence> seq;
        RRELBrackets(std::unique_ptr<RRELSequence> &&seq) : seq{std::move(seq)} {}
        void print(std::ostream& o) const override;
        cppcoro::generator<const RRELInternalResult> get_next_matches(
            std::shared_ptr<textx::object::Object>, 
            std::vector<std::string> lookup_list,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
    };

    struct RRELDots : RRELPathElement {
        size_t n;
        RRELDots(size_t n) : n{n} {}
        void print(std::ostream& o) const override;
        cppcoro::generator<const RRELInternalResult> get_next_matches(
            std::shared_ptr<textx::object::Object>, 
            std::vector<std::string> lookup_list,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
    };

    struct RRELPath;
    struct RRELSequence : RRELBase {
        std::vector<std::unique_ptr<RRELPath>> paths;
        RRELSequence(std::vector<std::unique_ptr<RRELPath>> &&paths) : paths{std::move(paths)} {};
        void print(std::ostream& o) const override;
        cppcoro::generator<const RRELInternalResult> get_next_matches(
            std::shared_ptr<textx::object::Object>, 
            std::vector<std::string> lookup_list,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
    };

    struct RRELZeroOrMore : RRELPathElement {
        std::unique_ptr<RRELPathElement> path_element;
        RRELZeroOrMore(std::unique_ptr<RRELPathElement> path_element) : path_element{std::move(path_element)} {}
        void print(std::ostream& o) const override;
        cppcoro::generator<const RRELInternalResult> get_next_matches(
            std::shared_ptr<textx::object::Object>, 
            std::vector<std::string> lookup_list,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
    };

    struct RRELPath : RRELBase {
        std::vector<std::unique_ptr<RRELPathElement>> path_elements;
        RRELPath(std::vector<std::unique_ptr<RRELPathElement>> &&path_elements) : path_elements{std::move(path_elements)} {};       
        void print(std::ostream& o) const override;
        cppcoro::generator<const RRELInternalResult> get_next_matches(
            std::shared_ptr<textx::object::Object>, 
            std::vector<std::string> lookup_list,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
    };

    struct RRELExpression : RRELBase {
        std::unique_ptr<RRELSequence> seq;
        std::string flags;
        bool use_proxy;
        // note: importURI handled differently (ignored here)
        RRELExpression(std::unique_ptr<RRELSequence> &&seq, bool use_proxy, std::string flags) : seq{std::move(seq)}, use_proxy{use_proxy}, flags{flags} {}
        void print(std::ostream& o) const override;
        cppcoro::generator<const RRELInternalResult> get_next_matches(
            std::shared_ptr<textx::object::Object>, 
            std::vector<std::string> lookup_list,
            AllowedFunc allowed,
            bool first_element=false
        ) const override;
    };

    std::unique_ptr<RRELExpression> create_RREL_expression(textx::arpeggio::Match m);
    std::unique_ptr<RRELExpression> create_RREL_expression(std::string rrel_expression_string);

    RRELResult find_object_with_path(std::shared_ptr<textx::object::Object> obj, std::vector<std::string> lookup, textx::rrel::RRELExpression& rrel_tree, std::string obj_cls="");
    inline RRELResult find_object_with_path(
        std::shared_ptr<textx::object::Object> obj,
        std::variant<std::string,std::vector<std::string>> lookup,
        textx::rrel::RRELExpression& rrel_tree, 
        std::string obj_cls="", 
        std::string split_string=".")
    {
        std::vector<std::string> new_lookup {};
        if (std::holds_alternative<std::string>(lookup)) {
            TEXTX_ASSERT(split_string.size()==1);
            std::string& str = std::get<std::string>(lookup);
            char delim = split_string[0];
            size_t start;
            size_t end = 0;
        	while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
            {
                end = str.find(delim, start);
                new_lookup.push_back(str.substr(start, end - start));
            }
        }
        else {
            new_lookup = std::move(std::get<std::vector<std::string>>(lookup));
        }
        return find_object_with_path(obj, new_lookup, rrel_tree, obj_cls);
    }
    inline RRELResult find_object_with_path(
        std::shared_ptr<textx::object::Object> obj,
        std::variant<std::string,std::vector<std::string>> lookup,
        std::string rrel_tree, 
        std::string obj_cls="", 
        std::string split_string=".") 
    {
        auto rrel = create_RREL_expression(rrel_tree);
        return find_object_with_path(obj, lookup, *rrel, obj_cls, split_string);
    }
}