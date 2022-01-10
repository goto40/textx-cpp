#include "itemlang.h"
#include <iostream>

int main() {
    try {
        auto workspace = itemlang::get_itemlang_metamodel_workspace();

    }
    catch(std::exception& e) {
        std::cerr << e.what();
        return 2;
    }
    return 0;
}