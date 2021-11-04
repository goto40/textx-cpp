#pragma once

#include <stdexcept>
#include <sstream>

namespace textx {
    template<class T, class U>
    void textx_assert_equal(const char *filename, size_t lineno, std::string assert_text, const T& a, const U& b, std::string info="") {
        if (a!=b) {
            std:: ostringstream o;
            o << assert_text << " failed, " << (a) << "!=" << (b) << " " << info << " (" << filename << ":" << lineno << ")";
            throw std::runtime_error(o.str());
        }
    }
    template<class ...T>
    inline void textx_assert(const char *filename, size_t lineno, std::string assert_text, bool a, T... info) {
        if (!a) {
            std:: ostringstream o;
            o << assert_text << " failed, ";
            (o << ... << info) << " (" << filename << ":" << lineno << ")";
            throw std::runtime_error(o.str());
        }
    }
}
#define TEXTX_STR(a) #a
#define TEXTX_ASSERT_EQUAL(a,b,...) ::textx::textx_assert_equal(__FILE__, __LINE__, TEXTX_STR((a)==(b)),(a),(b) __VA_OPT__(, __VA_ARGS__));
#define TEXTX_ASSERT(a, ...) ::textx::textx_assert(__FILE__, __LINE__, TEXTX_STR((a)), (a) __VA_OPT__(, __VA_ARGS__));
