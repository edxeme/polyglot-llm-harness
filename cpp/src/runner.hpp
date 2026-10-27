#pragma once

#include <string>
#include <vector>

namespace lmh {

// Exit codes shared across implementations.
constexpr int EXIT_PASS = 0;
constexpr int EXIT_CASE_FAILURES = 1;
constexpr int EXIT_BUILD_FAILURE = 2;

// `run <task> [--candidate <file>] [--json]` starting at args[1].
int run_task(const std::vector<std::string>& args);

}  // namespace lmh
