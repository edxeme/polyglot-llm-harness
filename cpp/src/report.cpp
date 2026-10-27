#include "report.hpp"

#include <cstdio>

namespace lmh {

std::string CaseResult::to_json() const {
    return "{\"name\":\"" + escape(name) + "\",\"status\":\"" + status + "\",\"ms\":" + std::to_string(ms) +
           ",\"detail\":\"" + escape(detail) + "\"}";
}

std::string Report::to_json() const {
    char buf[128];
    snprintf(buf, sizeof buf, "%.3f", duration_ms);
    std::string out = "{\"schema\":\"lmh.report/v1\",\"task\":\"" + escape(task) + "\",\"impl\":\"" + impl +
                      "\",\"source\":\"" + escape(source) + "\",\"duration_ms\":" + buf;
    snprintf(buf, sizeof buf, ",\"summary\":{\"total\":%d,\"passed\":%d,\"failed\":%d},\"score\":%.3f,\"cases\":[",
             total, passed, failed(), score());
    out += buf;
    for (size_t i = 0; i < cases.size(); ++i) {
        if (i > 0) out += ',';
        out += cases[i].to_json();
    }
    out += "]";
    if (!build_error.empty()) {
        out += ",\"build_error\":\"" + escape(build_error) + "\"";
    }
    return out + "}";
}

std::string Report::to_human() const {
    std::string out = "task " + task + " · impl " + impl + " · source " + source + "\n";
    char buf[48];
    for (const auto& c : cases) {
        snprintf(buf, sizeof buf, "  %-4s %-28s %8.3fms", c.pass() ? "pass" : "FAIL", c.name.c_str(), c.ms);
        out += buf;
        if (!c.pass() && !c.detail.empty()) {
            out += "  " + c.detail;
        }
        out += "\n";
    }
    if (!build_error.empty()) {
        out += "  build error: " + build_error + "\n";
    }
    snprintf(buf, sizeof buf, "%d/%d passed · score %.3f · %.1fms\n", passed, total, score(), duration_ms);
    return out + buf;
}

}  // namespace lmh
