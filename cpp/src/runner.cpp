#include "runner.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "json.hpp"
#include "registry.hpp"
#include "report.hpp"

namespace fs = std::filesystem;

namespace lmh {

namespace {

struct Exec {
    int exit = 0;
    std::string stdout_;
    std::string stderr_;
};

std::string shell_quote(const std::string& arg) {
    return '"' + arg + '"';
}

// Runs a command with merged stdout/stderr capture (build step).
Exec exec_merged(const std::vector<std::string>& cmd) {
    std::string joined;
    for (size_t i = 0; i < cmd.size(); ++i) {
        if (i > 0) joined += ' ';
        joined += shell_quote(cmd[i]);
    }
    std::string redirected = joined + " 2>&1";
    FILE* pipe = popen(redirected.c_str(), "r");
    if (!pipe) throw std::runtime_error("cannot spawn: " + joined);
    std::string out;
    char buf[4096];
    while (fgets(buf, sizeof buf, pipe) != nullptr) out += buf;
    int status = pclose(pipe);
    return Exec{WIFEXITED(status) ? WEXITSTATUS(status) : -1, out, ""};
}

// Runs a command capturing stdout (parseable JSONL); stderr goes to a file.
Exec exec_split(const std::vector<std::string>& cmd, const fs::path& stderr_file) {
    std::string joined;
    for (size_t i = 0; i < cmd.size(); ++i) {
        if (i > 0) joined += ' ';
        joined += shell_quote(cmd[i]);
    }
    std::string redirected = joined + " 2>" + stderr_file.string();
    FILE* pipe = popen(redirected.c_str(), "r");
    if (!pipe) throw std::runtime_error("cannot spawn verifier");
    std::string out;
    char buf[4096];
    while (fgets(buf, sizeof buf, pipe) != nullptr) out += buf;
    int status = pclose(pipe);
    std::ifstream err_in(stderr_file);
    std::stringstream ss;
    ss << err_in.rdbuf();
    return Exec{WIFEXITED(status) ? WEXITSTATUS(status) : -1, out, ss.str()};
}

std::string read_file(const fs::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot read " + path.string());
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string first_line(const std::string& s) {
    std::istringstream iss(s);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty()) return line;
    }
    return "";
}

struct DriveOut {
    std::vector<CaseResult> cases;
    int total = 0;
    int passed = 0;
    std::string build_error;  // empty on success
};

double num_or_zero(const std::map<std::string, Value>* obj, const std::string& key) {
    if (!obj) return 0.0;
    auto it = obj->find(key);
    return it != obj->end() && it->second.kind == Value::Kind::NUM ? it->second.num : 0.0;
}

std::string read_cases_trimmed(const std::string& root, const TaskSpec& spec) {
    std::string cases = spec.cases(root);
    const auto first = cases.find_first_not_of(" \t\r\n");
    const auto last = cases.find_last_not_of(" \t\r\n");
    return first == std::string::npos ? "" : cases.substr(first, last - first + 1);
}

DriveOut drive(const TaskSpec& spec, const std::string& root, const std::string& source, const fs::path& tmp) {
    fs::path template_path = fs::path(root) / fs::path(spec.cases_path).parent_path().parent_path() / "driver.cpp.template";
    std::string driver = read_file(template_path);

    size_t marker = driver.find("// CASES");
    if (marker == std::string::npos) throw std::runtime_error("driver template missing // CASES marker");
    driver.replace(marker, std::string("// CASES").size(), read_cases_trimmed(root, spec));

    std::ofstream candidate(tmp / "candidate.hpp");
    candidate << source;
    candidate.close();
    std::ofstream verifier(tmp / "verifier.cpp");
    verifier << driver;
    verifier.close();

    Exec build = exec_merged({"c++", "-std=c++17", "-O2", "-o", (tmp / "verifier_bin").string(),
                              (tmp / "verifier.cpp").string()});
    if (build.exit != 0) {
        DriveOut out;
        out.build_error = first_line(build.stdout_).empty() ? "compile failed" : first_line(build.stdout_);
        return out;
    }

    Exec run = exec_split({(tmp / "verifier_bin").string()}, tmp / "verifier.stderr");

    DriveOut out;
    std::istringstream iss(run.stdout_);
    std::string line;
    while (std::getline(iss, line)) {
        const auto trimmed = line.find_first_not_of(" \t\r\n");
        if (trimmed == std::string::npos || line[trimmed] != '{') continue;
        std::map<std::string, Value> obj;
        try {
            obj = parse_object(line.substr(trimmed));
        } catch (const std::exception&) {
            continue;
        }
        auto sum = obj.find("summary");
        if (sum != obj.end() && sum->second.kind == Value::Kind::OBJ) {
            out.total = static_cast<int>(num_or_zero(&sum->second.obj, "total"));
            out.passed = static_cast<int>(num_or_zero(&sum->second.obj, "passed"));
            continue;
        }
        auto name_it = obj.find("case");
        if (name_it != obj.end() && name_it->second.kind == Value::Kind::STR) {
            CaseResult c;
            c.name = name_it->second.str;
            auto st = obj.find("status");
            c.status = st != obj.end() && st->second.kind == Value::Kind::STR ? st->second.str : "fail";
            c.ms = num_or_zero(&obj, "ms");
            auto d = obj.find("detail");
            c.detail = d != obj.end() && d->second.kind == Value::Kind::STR ? d->second.str : "";
            out.cases.push_back(std::move(c));
        }
    }
    if (run.exit == 0 || run.exit == 1) {
        // success path: build_error stays empty
    } else {
        out.build_error = "verifier exited with " + std::to_string(run.exit);
    }
    return out;
}

int finish(const TaskSpec& spec, const std::string& source_label, double duration_ms, const DriveOut& out, bool json) {
    Report report;
    report.task = spec.id;
    report.source = source_label;
    report.duration_ms = duration_ms;
    report.cases = out.cases;
    report.total = out.total;
    report.passed = out.passed;
    report.build_error = out.build_error;
    std::cout << (json ? report.to_json() : report.to_human()) << "\n";
    if (!out.build_error.empty()) return EXIT_BUILD_FAILURE;
    return report.failed() > 0 ? EXIT_CASE_FAILURES : EXIT_PASS;
}

}  // namespace

int run_task(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "lmh: run requires a task id (see 'lmh tasks')\n";
        return 3;
    }
    std::string candidate_path;
    bool use_golden = true;
    bool json = false;
    for (size_t i = 2; i < args.size(); ++i) {
        if (args[i] == "--candidate") {
            if (i + 1 >= args.size()) {
                std::cerr << "lmh: --candidate requires a path\n";
                return 3;
            }
            candidate_path = args[++i];
            use_golden = false;
        } else if (args[i] == "--json") {
            json = true;
        } else {
            std::cerr << "lmh: unknown option: " << args[i] << "\n";
            return 3;
        }
    }
    const TaskSpec* spec = find(args[1]);
    if (!spec) {
        std::cerr << "lmh: unknown task: " << args[1] << " (see 'lmh tasks')\n";
        return 3;
    }

    auto t0 = std::chrono::steady_clock::now();
    try {
        std::string root = find_root();
        std::string source = spec->source(root, use_golden, candidate_path);

        fs::path tmp = fs::temp_directory_path() /
                       ("lmh-" + std::to_string(::getpid()) + "-" + std::to_string(
                                     std::chrono::system_clock::now().time_since_epoch().count()));
        fs::create_directories(tmp);

        DriveOut out;
        try {
            out = drive(*spec, root, source, tmp);
        } catch (const std::exception& e) {
            out.build_error = e.what();
        }
        fs::remove_all(tmp);

        auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        std::string label = use_golden ? "golden" : fs::weakly_canonical(candidate_path).string();
        return finish(*spec, label, elapsed, out, json);
    } catch (const std::exception& e) {
        std::cerr << "lmh: " << e.what() << "\n";
        return 2;
    }
}

}  // namespace lmh
