#pragma once

#include <string>
#include <vector>

namespace lmh {

struct TaskSpec {
    std::string id;
    std::string candidate_file_name = "candidate.hpp";
    // repo-root-relative path of the case table fragment
    std::string cases_path;

    // Reads the candidate source: the golden bundle or a user file.
    std::string source(const std::string& root, bool use_golden, const std::string& candidate_path) const;
    std::string cases(const std::string& root) const;
};

extern const std::vector<TaskSpec> ALL;

const TaskSpec* find(const std::string& id);

// Resolves the repo root by walking up from cwd (tasks/ + docs/spec.md).
std::string find_root();

}  // namespace lmh
