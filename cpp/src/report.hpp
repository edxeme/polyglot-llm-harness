#pragma once

// lmh.report/v1 — the same shape every implementation emits.

#include <string>
#include <vector>

#include "json.hpp"

namespace lmh {

struct CaseResult {
    std::string name;
    std::string status;
    double ms = 0.0;
    std::string detail;

    bool pass() const { return status == "pass"; }
    std::string to_json() const;
};

struct Report {
    std::string task;
    std::string impl = "cpp";
    std::string source;
    double duration_ms = 0.0;
    std::vector<CaseResult> cases;
    int total = 0;
    int passed = 0;
    std::string build_error;  // empty when the verifier built and ran

    int failed() const { return total - passed; }
    double score() const { return total == 0 ? 0.0 : static_cast<double>(passed) / total; }
    std::string to_json() const;
    std::string to_human() const;
};

}  // namespace lmh
