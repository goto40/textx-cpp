#include <textx/metamodel.h>
#include <textx/assert.h>
#include <cstdint>

// https://adventofcode.com/2021
// day2b

int main(int argc, const char** argv) {
    try {
        TEXTX_ASSERT(argc==2, "pass one arg (filename)");

        auto mm = textx::metamodel_from_str(R"(
            Model: actions+=Action;
            Action: dir=Dir x=INT;
            Dir: 'forward'|'up'|'down';
        )");

        auto m = mm->model_from_file(argv[1]);
        int64_t h=0;
        int64_t d=0;
        int64_t a=0;

        std::unordered_map<std::string, std::function<void(int)>> f = {
            {"forward", [&](int x){ h+=x; d+=(x*a); }},
            {"up",      [&](int x){ a-=x; }},
            {"down",    [&](int x){ a+=x; }},
        };

        for (auto &a: m->val()["actions"]) {
            //std::cout << a["dir"].str() << " " << a["x"].i() << "\n";
            f[a["dir"].str()](a["x"].i());
        }
        std::cout << h << "=h d=" << d << " --> " << h*d << "\n";
    }
    catch(std::exception &e) {
        std::cout << "error: " << e.what() << "\n";
    }
}