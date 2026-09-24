#ifndef JSON_H
#define JSON_H

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <cctype>

enum class JsonType { Null, Boolean, Number, String, Array, Object };

struct JsonValue {
    JsonType type = JsonType::Null;
    bool bool_val = false;
    double num_val = 0.0;
    std::string str_val;
    std::vector<JsonValue> arr_val;
    std::map<std::string, JsonValue> obj_val;

    std::string serialize() const {
        switch (type) {
            case JsonType::Null: return "null";
            case JsonType::Boolean: return bool_val ? "true" : "false";
            case JsonType::Number: return std::to_string(num_val);
            case JsonType::String: return "\"" + str_val + "\"";
            case JsonType::Array: {
                std::string res = "[";
                for (size_t i = 0; i < arr_val.size(); ++i) {
                    if (i > 0) res += ",";
                    res += arr_val[i].serialize();
                }
                return res + "]";
            }
            case JsonType::Object: {
                std::string res = "{";
                size_t i = 0;
                for (auto& [k, v] : obj_val) {
                    if (i++ > 0) res += ",";
                    res += "\"" + k + "\":" + v.serialize();
                }
                return res + "}";
            }
        }
        return "null";
    }
};

class JsonParser {
    std::string src;
    size_t pos = 0;

    void skip_whitespace() {
        while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\r' || src[pos] == '\n')) {
            pos++;
        }
    }

    char peek() { skip_whitespace(); return pos < src.size() ? src[pos] : '\0'; }
    char get() { skip_whitespace(); return pos < src.size() ? src[pos++] : '\0'; }

    std::string parse_string() {
        get(); // пропустить начальную кавычку
        std::string res;
        while (pos < src.size()) {
            char c = src[pos++];
            if (c == '"') return res;
            if (c == '\\' && pos < src.size()) c = src[pos++];
            res += c;
        }
        throw std::runtime_error("Unterminated string in JSON");
    }

    JsonValue parse_number() {
        size_t start = pos;
        if (src[pos] == '-') pos++;
        while (pos < src.size() && (isdigit(src[pos]) || src[pos] == '.')) pos++;
        double val = std::stod(src.substr(start, pos - start));
        JsonValue v; v.type = JsonType::Number; v.num_val = val;
        return v;
    }

public:
    explicit JsonParser(std::string s) : src(std::move(s)) {}

    JsonValue parse_value() {
        skip_whitespace();
        char c = peek();
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') {
            JsonValue v; v.type = JsonType::String; v.str_val = parse_string();
            return v;
        }
        if (isdigit(c) || c == '-') return parse_number();
        if (c == 't' || c == 'f') {
            JsonValue v; v.type = JsonType::Boolean;
            if (src.substr(pos, 4) == "true") { pos += 4; v.bool_val = true; }
            else if (src.substr(pos, 5) == "false") { pos += 5; v.bool_val = false; }
            return v;
        }
        return JsonValue{};
    }

    JsonValue parse_object() {
        get(); // '{'
        JsonValue obj; obj.type = JsonType::Object;
        if (peek() == '}') { get(); return obj; }
        while (true) {
            if (peek() != '"') break;
            std::string key = parse_string();
            if (get() != ':') break;
            obj.obj_val[key] = parse_value();
            char c = peek();
            if (c == ',') { get(); continue; }
            if (c == '}') { get(); break; }
        }
        return obj;
    }

    JsonValue parse_array() {
        get(); // '['
        JsonValue arr; arr.type = JsonType::Array;
        if (peek() == ']') { get(); return arr; }
        while (true) {
            arr.arr_val.push_back(parse_value());
            char c = peek();
            if (c == ',') { get(); continue; }
            if (c == ']') { get(); break; }
        }
        return arr;
    }
};

#endif