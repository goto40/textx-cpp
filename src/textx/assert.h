#pragma once

#include <stdexcept>
#include <sstream>

namespace textx {
    template<class T, class U>
    void textx_assert_equal(std::string assert_text, const T& a, const U& b, std::string info="") {
        if (a!=b) {
            std:: ostringstream o;
            o << assert_text << " failed, " << (a) << "!=" << (b) << " " << info;
            throw std::runtime_error(o.str());
        }
    }
    void textx_assert(std::string assert_text, bool a, std::string info="") {
        if (!a) {
            std:: ostringstream o;
            o << assert_text << " failed, " << info;
            throw std::runtime_error(o.str());
        }
    }
}
#define TEXTX_STR(a) #a
#define TEXTX_ASSERT_EQUAL(a,b,...) ::textx::textx_assert_equal(TEXTX_STR((a)==(b)),(a),(b) __VA_OPT__(, __VA_ARGS__));
#define TEXTX_ASSERT(a, ...) ::textx::textx_assert(TEXTX_STR((a)), (a) __VA_OPT__(, __VA_ARGS__));
