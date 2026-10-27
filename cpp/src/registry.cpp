#include "registry.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace lmh {

std::string TaskSpec::source(const std::string& root, bool use_golden, const std::string& candidate_path) const {
    fs::path path = use_golden ? fs::path(root) / "cpp" / "golden" / (id + ".hpp") : fs::path(candidate_path);
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot read " + path.string());
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string TaskSpec::cases(const std::string& root) const {
    fs::path path = fs::path(root) / cases_path;
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot read " + path.string());
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

const std::vector<TaskSpec> ALL = {
    {"merge-intervals", "candidate.hpp", "cpp/verifier/cases/merge-intervals.cases"},
    {"lru-cache", "candidate.hpp", "cpp/verifier/cases/lru-cache.cases"},
    {"rate-limiter", "candidate.hpp", "cpp/verifier/cases/rate-limiter.cases"},
};

const TaskSpec* find(const std::string& id) {
    for (const auto& t : ALL) {
        if (t.id == id) return &t;
    }
    return nullptr;
}

std::string find_root() {
    std::error_code ec;
    fs::path dir = fs::current_path(ec);
    while (!ec) {
        if (fs::is_directory(dir / "tasks") && fs::is_regular_file(dir / "docs" / "spec.md")) {
            return dir.string();
        }
        fs::path parent = dir.parent_path();
        if (parent == dir) break;
        dir = parent;
        ec.clear();
    }
    throw std::runtime_error("not inside an lmh checkout (no tasks/ and docs/spec.md upward)");
}

}  // namespace lmh
