#pragma once
#include <coroutine>
#include <type_traits>
#include <exception>
#include <utility>
#include <functional>

namespace textx::utils
{
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator () (const std::pair<T1,T2> &p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ h2;  
        }
    };    

    template<class T, class U>
    bool is_instance(U& obj) { return dynamic_cast<T*>(&obj)!=nullptr; }

    /* adapted from https://www.scs.stanford.edu/~dm/blog/c++-coroutines.html
       (added for-each begin/end support and other minor points) */
    template <typename T>
    struct Generator
    {
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;

        struct promise_type
        {
            std::remove_const_t<T> value_;
            std::exception_ptr exception_;

            Generator get_return_object()
            {
                return Generator(handle_type::from_promise(*this));
            }
            std::suspend_always initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            void unhandled_exception() { exception_ = std::current_exception(); }
            template <std::convertible_to<T> From> // C++20 concept
            std::suspend_always yield_value(From &&from)
            {
                value_ = std::forward<From>(from);
                return {};
            }
            void return_void() {}
        };

        handle_type h_;

        Generator(handle_type h) : h_(h) {}
        ~Generator() { h_.destroy(); }
        explicit operator bool()
        {
            fill();
            return !h_.done();
        }
        T operator()()
        {
            fill();
            full_ = false;
            return std::move(h_.promise().value_);
        }

        struct Iterator
        {
            Generator &g;
            bool operator!=(std::nullptr_t) { return static_cast<bool>(g); }
            void operator++() {}
            T operator*() { return g(); }
        };
        Iterator begin() { return Iterator{*this}; }
        std::nullptr_t end() { return nullptr; }

    private:
        bool full_ = false;

        void fill()
        {
            if (!full_)
            {
                h_();
                if (h_.promise().exception_)
                    std::rethrow_exception(h_.promise().exception_);
                full_ = true;
            }
        }
    };

}