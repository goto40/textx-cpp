#include "itemlang.h"
#include <iostream>

int main(int argc, const char** argv) {
    TEXTX_ASSERT(argc==2);
    try {
        auto workspace = itemlang::get_itemlang_metamodel_workspace();
        auto m = workspace->model_from_file(argv[1]);
        std::cout << m->val() << "\n";
    }
    catch(std::exception& e) {
        std::cerr << e.what();
        return 2;
    }
    return 0;
}