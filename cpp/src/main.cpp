#include <iostream>
#include <string>
#include <vector>

#include "registry.hpp"
#include "runner.hpp"

namespace {

const char* USAGE =
    "lmh — polyglot llm verifier harness (c++ implementation)\n\n"
    "usage:\n"
    "  lmh tasks\n"
    "  lmh run <task> [--candidate <file>] [--json]\n\n"
    "exit codes: 0 pass, 1 case failures, 2 build failure, 3 usage\n";

}  // namespace

int main(int argc, char** argv) {
    std::vector<std::string> args(argv, argv + argc);
    if (args.size() <= 1) {
        std::cerr << USAGE;
        return 3;
    }
    const std::string& cmd = args[1];
    if (cmd == "-h" || cmd == "--help" || cmd == "help") {
        std::cout << USAGE;
        return 0;
    }
    if (cmd == "tasks") {
        for (const auto& t : lmh::ALL) {
            printf("%-16s candidate %s\n", t.id.c_str(), t.candidate_file_name.c_str());
        }
        return 0;
    }
    if (cmd == "run") {
        try {
            return lmh::run_task(std::vector<std::string>(args.begin() + 1, args.end()));
        } catch (const std::exception& e) {
            std::cerr << "lmh: " << e.what() << "\n";
            return 3;
        }
    }
    std::cerr << "lmh: unknown command: " << cmd << "\n";
    return 3;
}
