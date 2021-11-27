#include "textx/parsetree.h"
#include "textx/metamodel.h"
#include "textx/tools.h"

namespace textx::parsetree {

    void RuleInfo::fix_tx_inh_by(ParseTree& p) {
        // remove_match_types_from
        std::erase_if(tx_inh_by, [&](auto&x){ return p[x].rule_type==RuleType::match; });
        // adjust bases
        for (auto& n: tx_inh_by ) {
            p[n].tx_bases.insert(name); // add myself as base class
        }
    }

    void RuleInfo::fix_attribute_types(const ParseTree& p) {
        for (auto& [name, info]: attribute_info) {
            std::function<bool(std::string,std::string)> is_inherited_from;
            is_inherited_from = [&](std::string t1, std::string t2) -> bool {
                if (t1==t2) return true;
                for (auto &b: p[t1].tx_bases) {
                    if (is_inherited_from(b,t2)) return true;
                }
                return false;
            };
            auto common = [&](std::string t1, std::string t2) -> std::string {
                if (t1==t2) return t1;
                if (is_inherited_from(t1,t2)) return t2;
                std::unordered_set<std::string> bases1={t2};
                std::unordered_set<std::string> bases2={};
                while(bases1.size()>0) {
                    for (auto &b: bases1) {
                       for (auto &n: p[b].tx_bases) {
                            bases2.insert(n);
                            if (is_inherited_from(t1,n)) return n;
                       }
                    }
                    std::swap(bases1, bases2);
                }
                return "OBJECT";
            };

            // remove doubles
            auto u = std::unique(info.types.begin(), info.types.end());
            info.types.erase(u,info.types.end());
            // remove match rules
            std::erase_if(info.types, [&](auto&x){ return p[x].rule_type==RuleType::match; });
            // now find common rule
            if (info.types.size()>0) {
                info.type = info.types[0];
                for (auto&t: info.types) {
                    info.type = common(info.type.value(),t);
                }
            }
            else {
                // nothing to do (no type)
                info.type = std::nullopt;
            }
        }
    }

    textx::RuleType RuleInfo::determine_rule_type(std::unordered_set<std::string> &recursion_stopper, const ParseTree& p) const {
        if (recursion_stopper.count(name)>0) {
            throw std::runtime_error("detected circular abstract rule reference");
        }
        recursion_stopper.insert(name);
        textx::OnExit onexit{ [&](){ recursion_stopper.erase(name); } };

        // TODO determine_rule_type, see http://textx.github.io/textX/stable/grammar/#rule-types
        if (attribute_info.size()>0) {
            return textx::RuleType::common;
        }
        else if (std::count_if(
            tx_inh_by.begin(),
            tx_inh_by.end(),
            [&](auto &n){
                return p[n].determine_rule_type(recursion_stopper, p) != RuleType::match;
            })>0) {
            return textx::RuleType::abstract;
        }
        else {
            return textx::RuleType::match;
        }
    }

    void ParseTree::finalize_rule_info() {
        std::unordered_set<std::string> recursion_stopper{};
        for (auto&[name,r] : rule_info) {
            r.rule_type = r.determine_rule_type(recursion_stopper, *this);
            r.fix_tx_inh_by(*this);
        }
        for (auto&[name,r] : rule_info) {
            r.fix_attribute_types(*this);
        }
    }
}