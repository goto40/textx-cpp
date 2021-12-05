#include "textx/model.h"
#include "textx/metamodel.h"
#include "textx/arpeggio.h"

namespace textx {

    void Model::init(const std::string_view text, const textx::arpeggio::Match &parsetree, std::shared_ptr<Metamodel> mm) {
       weak_mm = mm;
       root = create_model(text, parsetree, *mm);
    }

    textx::object::Value Model::create_model(const std::string_view text, const textx::arpeggio::Match &m, textx::Metamodel &mm) {
        if (m.name_starts_with("rule://")) {
            std::string rule_name = m.name.value().substr(7);
            auto &rule = mm[rule_name];
            if (rule.type() == RuleType::match) {
                //std::cout << "match -*- " << m << "\n";
                return {textx::object::MatchText{textx::arpeggio::get_str(text, m),rule_name},m.start()};
            }
            if (rule.type() == RuleType::common) {
                return create_model_from_common_rule(rule_name, text, m, mm);
            }
            else {
                return create_model_from_abstract_rule(rule_name, text, m, mm);
            }
        }
        if (textx::arpeggio::is_terminal(m)) { // also a match
            return {textx::object::MatchText{textx::arpeggio::get_str(text, m), "?"},m.start()};
        }
        else {
            textx::arpeggio::raise(m.start(), "unexpected, no rule result found here to create object data...\n", m);
        }
    }

    textx::object::Value Model::create_model_from_common_rule(const std::string& rule_name, const std::string_view text, const textx::arpeggio::Match &m0, textx::Metamodel &mm) {
        auto obj = std::make_shared<textx::object::Object>();
        obj->type = rule_name;
        obj->weak_model = shared_from_this(); // store weak ptr

        // create all fields with empty content
        const auto &rule = mm[rule_name];
        for (auto &[attr_name,info]: rule.get_attribute_info()) {
            if (info.cardinality == AttributeCardinality::list) {
                obj->create_attribute_if_not_present(attr_name);
                //std::cout << "create list " << attr_name << "\n";
                (*obj)[attr_name] = textx::object::AttributeValue{std::vector<textx::object::Value>{}};
            }
            else if (info.cardinality == AttributeCardinality::scalar) {
                obj->create_attribute_if_not_present(attr_name);
                //std::cout << "create scalar " << attr_name << "\n";
                if (info.is_text()) {
                    std::string t="";
                    if (info.type.has_value()) {
                        t = info.type.value();
                    }
                    (*obj)[attr_name] = textx::object::AttributeValue{textx::object::Value{textx::object::MatchText{"",t},m0.start()}};
                }
                else {
                    (*obj)[attr_name] = textx::object::AttributeValue{textx::object::Value{std::shared_ptr<textx::object::Object>{}, m0.start()}};
                }
            }
            else {
                textx::arpeggio::raise(m0.start(), rule_name, attr_name, "unexpected attribute config found...");
            }
        }

        // traverse tree and stop on "rule://" names..."
        std::function<void(const textx::arpeggio::Match&, bool)> traverse;
        traverse = [&, this](const textx::arpeggio::Match& m, bool first=true) {
            if (m.name_starts_with("rule://") && !first) {
                return;
            }
            else if (m.name_starts_with("assignment://")) {
                std::string attr_name = m.name.value().substr(13);
                //std::cout << "*************** adding attr " << m.name.value() << "\n"<< m << "\n";
                auto& val = m.children[0];

                // reference assignment
                if (val.name_starts_with("obj_ref://")) {
                    std::string ref_name = std::string{textx::arpeggio::get_str(text, val.children[0])};
                    if (mm[rule_name][attr_name].cardinality==AttributeCardinality::scalar) {
                        (*obj)[attr_name].data = textx::object::Value{textx::object::ObjectRef{shared_from_this(), ref_name, rule_name, attr_name, obj}, val.start()};
                    }
                    else {
                        (*obj)[attr_name].append(textx::object::Value{textx::object::ObjectRef{shared_from_this(), ref_name, rule_name, attr_name, obj}, val.start()});
                    }
                }
                else { // no reference
                    if (mm[rule_name][attr_name].cardinality==AttributeCardinality::scalar) {
                        (*obj)[attr_name].data = create_model(text, val, mm);
                    }
                    else {
                        (*obj)[attr_name].append(create_model(text, val, mm));
                    }
                }
            }
            else {
                for (auto &c: m.children) {
                    traverse(c, false);
                }
            }
        };
        traverse(m0,true);

        //std::cout << m0 << "\n";
        return {obj, m0.start()};
    }

    textx::object::Value Model::create_model_from_abstract_rule(const std::string& rule_name, const std::string_view text, const textx::arpeggio::Match &m0, textx::Metamodel &mm) {
        // traverse tree and stop on "rule://" names..."
        std::function<const textx::arpeggio::Match&(const textx::arpeggio::Match&, bool)> traverse;
        traverse = [&, this](const textx::arpeggio::Match& m, bool first=true) -> const textx::arpeggio::Match&{
            if (m.name_starts_with("rule://") && !first) {
                return m;
            }
            else {
                for (auto &c: m.children) {
                    auto &r = traverse(c, false);
                    if (&r != &m0) return r;
                }
            }
            return m0;
        };
        auto &r = traverse(m0,true);
        if(&r == &m0) {
            //std::cout << "abstract rule --> match -*- " << m << "\n";
            return {textx::object::MatchText{textx::arpeggio::get_str(text, m0), rule_name},m0.start()};            
        }
        return create_model(text, r, mm);
    }

    size_t Model::resolve_references() {
        auto mm = weak_mm.lock();
        size_t unresolved=0;
        textx::object::traverse(root,[&](textx::object::Value& v) -> void {
            if (v.is_ref()) {
                if (v.ref().obj.lock() == nullptr) {
                    //resolve ref:
                    //std::cout << "searching " << v.ref().name << ".\n";
                    v.ref().obj = mm->get_resolver(v.ref().rule, v.ref().attr)
                        .resolve(v.ref().parent.lock(),v.ref().name);
                    if (v.ref().obj.lock() == nullptr) {
                        //std::cout << "not found.\n";
                        unresolved++;
                    }
                    // else {
                    //     std::cout << "found.\n";
                    // }
                }
            }
        });
        return unresolved;
    }

}