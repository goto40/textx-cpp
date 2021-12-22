#include "textx/utils.h"
#include <iostream>

using namespace textx::utils;

Generator<unsigned>
counter6()
{
    for (unsigned i = 0; i < 3;)
        co_yield i++;
}

int main()
{
    {
        auto gen = counter6();
        while (gen)
            std::cout << "counter6: " << gen() << std::endl;
    }
    {
        for (auto x: counter6()) {
            std::cout << "counter6-for: " << x << std::endl;
        }
    }
}