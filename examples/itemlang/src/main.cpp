#include "itemlang.h"
#include "textx/export.h"
#include <iostream>

int main(int argc, const char** argv) {
    TEXTX_ASSERT(argc==2);
    try {
        auto workspace = itemlang::get_itemlang_metamodel_workspace();
        auto m = workspace->model_from_file(argv[1]);
        //std::cout << m->val() << "\n";
        textx::save_model_as_json(m);
    }
    catch(std::exception& e) {
        std::cerr << e.what();
        return 2;
    }
    return 0;
}