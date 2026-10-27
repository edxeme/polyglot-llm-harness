#include "json.hpp"

#include <cctype>
#include <cstdio>
#include <stdexcept>

namespace lmh {

namespace {

struct Parser {
    const std::string& src;
    size_t pos = 0;

    void ws() {
        while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos]))) pos++;
    }

    char peek() {
        if (pos >= src.size()) throw std::runtime_error("unexpected end of input");
        return src[pos];
    }

    char next() {
        char c = peek();
        pos++;
        return c;
    }

    void expect(char c) {
        if (peek() != c) throw std::runtime_error(std::string("expected '") + c + "' at " + std::to_string(pos));
        pos++;
    }

    std::string string() {
        expect('"');
        std::string out;
        while (true) {
            char c = next();
            if (c == '"') return out;
            if (c != '\\') {
                out += c;
                continue;
            }
            char e = next();
            switch (e) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'u': {
                    if (pos + 4 > src.size()) throw std::runtime_error("bad \\u escape");
                    unsigned cp = 0;
                    for (int i = 0; i < 4; i++) {
                        char h = next();
                        cp <<= 4;
                        if (h >= '0' && h <= '9') cp += static_cast<unsigned>(h - '0');
                        else if (h >= 'a' && h <= 'f') cp += static_cast<unsigned>(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') cp += static_cast<unsigned>(h - 'A' + 10);
                        else throw std::runtime_error("bad \\u escape");
                    }
                    if (cp < 0x80) {
                        out += static_cast<char>(cp);
                    } else if (cp < 0x800) {
                        out += static_cast<char>(0xC0 | (cp >> 6));
                        out += static_cast<char>(0x80 | (cp & 0x3F));
                    } else {
                        out += static_cast<char>(0xE0 | (cp >> 12));
                        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (cp & 0x3F));
                    }
                    break;
                }
                default: throw std::runtime_error(std::string("bad escape \\") + e);
            }
        }
    }

    double number() {
        size_t start = pos;
        while (pos < src.size() && (std::isdigit(static_cast<unsigned char>(src[pos])) ||
                                    strchr("-+.eE", src[pos]) != nullptr)) {
            pos++;
        }
        if (start == pos) throw std::runtime_error("expected a number at " + std::to_string(pos));
        return std::stod(src.substr(start, pos - start));
    }

    Value value() {
        char c = peek();
        if (c == '{') {
            Value v;
            v.kind = Value::Kind::OBJ;
            v.obj = object();
            return v;
        }
        if (c == '"') {
            Value v;
            v.kind = Value::Kind::STR;
            v.str = string();
            return v;
        }
        Value v;
        v.kind = Value::Kind::NUM;
        v.num = number();
        return v;
    }

    std::map<std::string, Value> object() {
        expect('{');
        std::map<std::string, Value> map;
        ws();
        if (peek() == '}') {
            pos++;
            return map;
        }
        while (true) {
            ws();
            std::string key = string();
            ws();
            expect(':');
            ws();
            map[key] = value();
            ws();
            char c = next();
            if (c == '}') return map;
            if (c != ',') throw std::runtime_error("expected ',' or '}'");
        }
    }
};

}  // namespace

std::map<std::string, Value> parse_object(const std::string& line) {
    std::string trimmed = line;
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back()))) trimmed.pop_back();
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front()))) trimmed.erase(0, 1);
    Parser p{trimmed};
    p.ws();
    auto obj = p.object();
    p.ws();
    if (p.pos != trimmed.size()) throw std::runtime_error("trailing characters");
    return obj;
}

std::string escape(const std::string& s) {
    char buf[8];
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    snprintf(buf, sizeof buf, "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

}  // namespace lmh
