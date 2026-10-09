#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>

enum class JsonType { Null, Boolean, Number, String, Array, Object };

struct JsonValue {
    JsonType type = JsonType::Null;
    bool bool_val = false;
    double num_val = 0.0;
    std::string str_val;
    std::vector<JsonValue> arr_val;
    std::map<std::string, JsonValue> obj_val;

    bool is_null() const { return type == JsonType::Null; }
    bool is_bool() const { return type == JsonType::Boolean; }
    bool is_num() const { return type == JsonType::Number; }
    bool is_str() const { return type == JsonType::String; }
    bool is_arr() const { return type == JsonType::Array; }
    bool is_obj() const { return type == JsonType::Object; }

    const JsonValue& operator[](const std::string& key) const {
        static JsonValue null_v;
        if (type != JsonType::Object) return null_v;
        auto it = obj_val.find(key);
        return (it != obj_val.end()) ? it->second : null_v;
    }

    const JsonValue& operator[](size_t idx) const {
        static JsonValue null_v;
        if (type != JsonType::Array || idx >= arr_val.size()) return null_v;
        return arr_val[idx];
    }

    bool has(const std::string& key) const {
        return type == JsonType::Object && obj_val.find(key) != obj_val.end();
    }
};

class JsonParser {
public:
    static JsonValue parse(const std::string& json_str);
    static JsonValue parseFile(const std::string& filepath);
};
