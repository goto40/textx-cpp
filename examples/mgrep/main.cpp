#include <fstream>
#include <string>
#include <iostream>
#include <iterator>
#include <boost/program_options.hpp>
#include <exception>
#include <vector>

int main(int ac, char** av){
    namespace po = boost::program_options;
    po::options_description desc("Usage: mgrep [options] <model> <files>");
    desc.add_options()
        ("help", "produce help message")
        ("parse-model", po::value<std::string>()->required(), "set parse model")
        ("input-file", po::value< std::vector<std::string> >()->required(), "input file")
    ;
    po::positional_options_description p;
    p.add("parse-model", 1);
    p.add("input-file", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(ac, av).options(desc).positional(p).run(), vm);

    try {

        po::notify(vm);
        std::string parse_model={};
        if (vm.count("parse-model")) {
            std::cout << "parse-model = "
                << vm["parse-model"].as<std::string>() << ".\n";
            parse_model = vm["parse-model"].as<std::string>();
        }
        else {
            throw std::runtime_error("parse-model was not set.");
        }

        if (vm.count("input-file")) {
            auto input_file = vm["input-file"].as<std::vector<std::string>>();
            for (const auto& file: input_file) {
                //std::cout << "file: " << file << "\n";
            }
        }
        else {
            throw std::runtime_error("input-file was not set.");
        }
    }
    catch(std::exception& e) {
        if (vm.count("help")) {
            std::cout << desc << "\n";
            return EXIT_SUCCESS;
        }
        std::cerr << "error: " << e.what() << "\n";
        std::cerr << desc << "\n";
        return EXIT_FAILURE;
    }
    catch(...) {
        std::cerr << "Exception of unknown type!\n";
        std::cerr << desc << "\n";
        return EXIT_FAILURE;
    }
}
