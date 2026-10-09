#include "json_parser.hpp"
#include <fstream>
#include <sstream>
#include <cctype>

class Tokenizer {
    std::string src;
    size_t pos = 0;

    void skipWhitespace() {
        while (pos < src.length() && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\n' || src[pos] == '\r')) {
            pos++;
        }
    }

    char peek() {
        skipWhitespace();
        return (pos < src.length()) ? src[pos] : '\0';
    }

    char get() {
        skipWhitespace();
        return (pos < src.length()) ? src[pos++] : '\0';
    }

    bool match(char expected) {
        if (peek() == expected) {
            get();
            return true;
        }
        return false;
    }

    std::string parseString() {
        if (get() != '"') throw std::runtime_error("Ожидалось начало строки ('\"')");
        std::string res;
        while (pos < src.length()) {
            char c = src[pos++];
            if (c == '"') return res;
            if (c == '\\' && pos < src.length()) {
                char esc = src[pos++];
                if (esc == 'n') res += '\n';
                else if (esc == 't') res += '\t';
                else res += esc;
            } else {
                res += c;
            }
        }
        throw std::runtime_error("Незавершенная строка в JSON");
    }

    double parseNumber() {
        size_t start = pos;
        if (src[pos] == '-') pos++;
        while (pos < src.length() && (std::isdigit(src[pos]) || src[pos] == '.')) {
            pos++;
        }
        return std::stod(src.substr(start, pos - start));
    }

public:
    Tokenizer(std::string s) : src(std::move(s)) {}

    JsonValue parseValue() {
        skipWhitespace();
        char c = peek();
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') {
            JsonValue v;
            v.type = JsonType::String;
            v.str_val = parseString();
            return v;
        }
        if (std::isdigit(c) || c == '-') {
            JsonValue v;
            v.type = JsonType::Number;
            v.num_val = parseNumber();
            return v;
        }
        if (c == 't' || c == 'f') {
            std::string w;
            while (pos < src.length() && std::isalpha(src[pos])) w += src[pos++];
            JsonValue v;
            v.type = JsonType::Boolean;
            v.bool_val = (w == "true");
            return v;
        }
        if (c == 'n') {
            std::string w;
            while (pos < src.length() && std::isalpha(src[pos])) w += src[pos++];
            JsonValue v;
            v.type = JsonType::Null;
            return v;
        }
        throw std::runtime_error(std::string("Некорректный символ в JSON: ") + c);
    }

    JsonValue parseObject() {
        get();
        JsonValue obj;
        obj.type = JsonType::Object;
        if (peek() == '}') { get(); return obj; }
        while (true) {
            std::string key = parseString();
            if (!match(':')) throw std::runtime_error("Ожидался символ ':' после ключа");
            obj.obj_val[key] = parseValue();
            if (match('}')) break;
            if (!match(',')) throw std::runtime_error("Ожидался символ ',' или '}'");
        }
        return obj;
    }

    JsonValue parseArray() {
        get();
        JsonValue arr;
        arr.type = JsonType::Array;
        if (peek() == ']') { get(); return arr; }
        while (true) {
            arr.arr_val.push_back(parseValue());
            if (match(']')) break;
            if (!match(',')) throw std::runtime_error("Ожидался символ ',' или ']'");
        }
        return arr;
    }
};

JsonValue JsonParser::parse(const std::string& json_str) {
    Tokenizer t(json_str);
    return t.parseValue();
}

JsonValue JsonParser::parseFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Ошибка открытия файла конфигурации: " + filepath);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return parse(ss.str());
}
