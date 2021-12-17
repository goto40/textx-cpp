#include <iostream>
#include <filesystem>
#include <cppcoro/generator.hpp>

//
// THIS IS AN EXAMPLE TO USE THE CPPCORO::GENERATOR
//

namespace fs = std::filesystem;

cppcoro::generator<const fs::directory_entry> list_directory(fs::path path) {
    for (const auto& entry: fs::directory_iterator(path)) {
        co_yield entry;
    }
}
cppcoro::generator<const fs::directory_entry> list_directory_recursive(fs::path path) {
    for(const auto& entry: list_directory(path)) {
        co_yield entry;
        if (fs::is_directory(entry)) {
            for (auto x: list_directory_recursive(entry.path())) {
                co_yield x;
            }
        }
    }
}

int main(int argc, const char** argv) {
    if (argc!=2) {
        std::cerr << "usage: " << argv[0] << " <dir>\n";
        return 1;
    }

    auto list_dir = list_directory_recursive(argv[1]);
    for (const auto& entry: list_dir) {
        std::cout << entry << "\n";
    }
}