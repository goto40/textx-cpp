#pragma once
#include <functional>
namespace textx {
    struct OnExit {
        std::function<void()> f;
        ~OnExit() { f(); }
    };
}
