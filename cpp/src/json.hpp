#pragma once

// Minimal JSON support for the verifier contract: a small recursive-descent
// parser over flat objects (with one nesting level for the summary map) plus
// string escaping for output. Dependency-free on purpose.

#include <map>
#include <string>

namespace lmh {

struct Value {
    enum class Kind { NUM, STR, OBJ } kind;
    double num = 0.0;
    std::string str;
    std::map<std::string, Value> obj;
};

// Parses one flat JSON object (the verifier contract shape).
std::map<std::string, Value> parse_object(const std::string& line);

// Escapes a string for embedding in a JSON document.
std::string escape(const std::string& s);

}  // namespace lmh
