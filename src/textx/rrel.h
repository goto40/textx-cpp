#pragma once
#include "textx/arpeggio.h"
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <sstream>

namespace textx::rrel {

    struct RRELBase {
        virtual ~RRELBase() = default;
        virtual void print(std::ostream& o)=0;
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
        void print(std::ostream& o) override;
    };

    struct RRELNavigation : RRELPathElement {
        std::string name;
        bool consume_name;
        RRELNavigation(std::string name, bool consume_name) : name{std::move(name)}, consume_name{consume_name} {}
        void print(std::ostream& o) override;
    };

    struct RRELSequence;
    struct RRELBrackets : RRELPathElement {
        std::unique_ptr<RRELSequence> seq;
        RRELBrackets(std::unique_ptr<RRELSequence> &&seq) : seq{std::move(seq)} {}
        void print(std::ostream& o) override;
    };

    struct RRELDots : RRELPathElement {
        size_t n;
        RRELDots(size_t n) : n{n} {}
        void print(std::ostream& o) override;
    };

    struct RRELPath;
    struct RRELSequence : RRELBase {
        std::vector<std::unique_ptr<RRELPath>> paths;
        RRELSequence(std::vector<std::unique_ptr<RRELPath>> &&paths) : paths{std::move(paths)} {};
        void print(std::ostream& o) override;
    };

    struct RRELZeroOrMore : RRELPathElement {
        std::unique_ptr<RRELPathElement> path_element;
        RRELZeroOrMore(std::unique_ptr<RRELPathElement> path_element) : path_element{std::move(path_element)} {}
        void print(std::ostream& o) override;
    };

    struct RRELPath : RRELBase {
        std::vector<std::unique_ptr<RRELPathElement>> path_elements;
        RRELPath(std::vector<std::unique_ptr<RRELPathElement>> &&path_elements) : path_elements{std::move(path_elements)} {};       
        void print(std::ostream& o) override;
    };

    struct RRELExpression : RRELBase {
        std::unique_ptr<RRELSequence> seq;
        std::string flags;
        bool use_proxy;
        // note: importURI handled differently (ignored here)
        RRELExpression(std::unique_ptr<RRELSequence> &&seq, bool use_proxy, std::string flags) : seq{std::move(seq)}, use_proxy{use_proxy}, flags{flags} {}
        void print(std::ostream& o) override;
    };

    std::unique_ptr<RRELExpression> create_RREL_expression(textx::arpeggio::Match m);
    std::unique_ptr<RRELExpression> create_RREL_expression(std::string rrel_expression_string);
}