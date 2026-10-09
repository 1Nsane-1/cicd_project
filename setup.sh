#!/bin/bash
set -e

echo "Создание структуры папок и файлов..."

mkdir -p .vscode src include plugins scripts bin deploy sample_app/src sample_app/tests

cat << 'EOF' > .vscode/tasks.json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "CMake: Build Project",
      "type": "shell",
      "command": "mkdir -p build && cd build && cmake .. && make -j$(nproc) && cp lib*.so .. 2>/dev/null || true",
      "group": { "kind": "build", "isDefault": true },
      "problemMatcher": "$gcc"
    },
    {
      "label": "CI/CD: Run Pipeline",
      "type": "shell",
      "command": "./build/ci-cd run --config pipeline.json",
      "dependsOn": "CMake: Build Project",
      "problemMatcher": []
    }
  ]
}
EOF

cat << 'EOF' > .vscode/launch.json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug CI/CD Pipeline",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/ci-cd",
      "args": ["run", "--config", "pipeline.json"],
      "stopAtEntry": false,
      "cwd": "${workspaceFolder}",
      "environment": [],
      "externalConsole": false,
      "MIMode": "gdb",
      "setupCommands": [
        {
          "description": "Enable pretty-printing for gdb",
          "text": "-enable-pretty-printing",
          "ignoreFailures": true
        }
      ],
      "preLaunchTask": "CMake: Build Project"
    }
  ]
}
EOF

cat << 'EOF' > CMakeLists.txt
cmake_minimum_required(VERSION 3.12)
project(cicd_project CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(ci-cd
    src/main.cpp
    src/json_parser.cpp
    src/pipeline.cpp
)
target_link_libraries(ci-cd PRIVATE ${CMAKE_DL_LIBS} pthread)

add_library(lint SHARED plugins/lint_plugin.cpp)
set_target_properties(lint PROPERTIES PREFIX "lib")

add_library(notify SHARED plugins/notify_plugin.cpp)
set_target_properties(notify PROPERTIES PREFIX "lib")
EOF

cat << 'EOF' > include/json_parser.hpp
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
EOF

cat << 'EOF' > src/json_parser.cpp
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
EOF

cat << 'EOF' > include/test_framework.hpp
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <future>
#include <mutex>
#include <sstream>

struct TestCase {
    std::string suite;
    std::string name;
    std::function<void(std::string&, bool&)> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }
    void addTest(const std::string& suite, const std::string& name, std::function<void(std::string&, bool&)> fn) {
        tests.push_back({suite, name, fn});
    }
    const std::vector<TestCase>& getTests() const { return tests; }
private:
    std::vector<TestCase> tests;
};

struct TestRegistrar {
    TestRegistrar(const std::string& suite, const std::string& name, std::function<void(std::string&, bool&)> fn) {
        TestRegistry::instance().addTest(suite, name, fn);
    }
};

#define TEST(suite_name, test_name) \
    void suite_name##_##test_name##_fn(std::string& _err_msg, bool& _passed); \
    static TestRegistrar suite_name##_##test_name##_reg(#suite_name, #test_name, suite_name##_##test_name##_fn); \
    void suite_name##_##test_name##_fn(std::string& _err_msg, bool& _passed)

#define ASSERT_EQ(val1, val2) do { \
    auto _v1 = (val1); \
    auto _v2 = (val2); \
    if (_v1 != _v2) { \
        std::ostringstream _ss; \
        _ss << "Сбой в " << __FILE__ << ":" << __LINE__ << " (" << #val1 << " [" << _v1 << "] != " << #val2 << " [" << _v2 << "])"; \
        _err_msg = _ss.str(); \
        _passed = false; \
        return; \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        std::ostringstream _ss; \
        _ss << "Сбой в " << __FILE__ << ":" << __LINE__ << " (" << #cond << " ложно)"; \
        _err_msg = _ss.str(); \
        _passed = false; \
        return; \
    } \
} while(0)

struct TestResult {
    std::string suite;
    std::string name;
    bool passed;
    std::string error;
};

inline int RunAllTestsParallel() {
    const auto& tests = TestRegistry::instance().getTests();
    std::vector<TestResult> results;
    std::mutex result_mutex;

    std::vector<std::future<void>> futures;
    for (const auto& test : tests) {
        futures.push_back(std::async(std::launch::async, [&test, &results, &result_mutex]() {
            std::string err;
            bool passed = true;
            try {
                test.func(err, passed);
            } catch (const std::exception& e) {
                passed = false;
                err = std::string("Исключение: ") + e.what();
            } catch (...) {
                passed = false;
                err = "Неизвестная ошибка";
            }
            std::lock_guard<std::mutex> lock(result_mutex);
            results.push_back({test.suite, test.name, passed, err});
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    int passed_cnt = 0, failed_cnt = 0;
    std::cout << "\n=== РЕЗУЛЬТАТЫ МОДУЛЬНОГО ТЕСТИРОВАНИЯ (std::async) ===\n";
    for (const auto& res : results) {
        if (res.passed) {
            std::cout << " [ УСПЕХ ] " << res.suite << "." << res.name << "\n";
            passed_cnt++;
        } else {
            std::cout << " [ ОШИБКА ] " << res.suite << "." << res.name << " -> " << res.error << "\n";
            failed_cnt++;
        }
    }
    std::cout << "-----------------------------------------------------\n";
    std::cout << "Пройдено: " << passed_cnt << " | Провалено: " << failed_cnt << "\n\n";
    return (failed_cnt == 0) ? 0 : 1;
}
EOF

cat << 'EOF' > include/plugin_interface.hpp
#pragma once
#include <string>
#include "json_parser.hpp"

class PipelinePlugin {
public:
    virtual ~PipelinePlugin() = default;
    virtual std::string getName() const = 0;
    virtual bool onStageStart(const std::string& stage_name) { (void)stage_name; return true; }
    virtual bool onStageEnd(const std::string& stage_name, bool success) { (void)stage_name; (void)success; return true; }
    virtual bool executeCustomStage(const std::string& stage_name, const JsonValue& config) {
        (void)stage_name; (void)config;
        return false;
    }
};

typedef PipelinePlugin* (*CreatePluginFn)();
EOF

cat << 'EOF' > include/plugin_manager.hpp
#pragma once
#include "plugin_interface.hpp"
#include <dlfcn.h>
#include <iostream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class PluginManager {
    struct LoadedPlugin {
        void* handle;
        PipelinePlugin* instance;
        std::string filename;
    };
    std::vector<LoadedPlugin> plugins;

public:
    ~PluginManager() { unloadAll(); }

    void loadPluginsFromDir(const std::string& dir_path) {
        if (!fs::exists(dir_path)) return;
        for (const auto& entry : fs::directory_iterator(dir_path)) {
            if (entry.path().extension() == ".so") {
                loadPlugin(entry.path().string());
            }
        }
    }

    bool loadPlugin(const std::string& filepath) {
        void* handle = dlopen(filepath.c_str(), RTLD_NOW);
        if (!handle) {
            std::cerr << "[PluginManager ERROR] dlopen: " << dlerror() << "\n";
            return false;
        }
        dlerror();
        CreatePluginFn create_fn = (CreatePluginFn)dlsym(handle, "create_plugin");
        const char* err = dlerror();
        if (err) {
            std::cerr << "[PluginManager ERROR] dlsym: " << err << "\n";
            dlclose(handle);
            return false;
        }
        PipelinePlugin* plugin = create_fn();
        if (plugin) {
            plugins.push_back({handle, plugin, filepath});
            std::cout << "[PluginManager] Загружен плагин: " << plugin->getName() << " (" << filepath << ")\n";
            return true;
        }
        dlclose(handle);
        return false;
    }

    void notifyStageStart(const std::string& stage_name) {
        for (auto& p : plugins) p.instance->onStageStart(stage_name);
    }

    void notifyStageEnd(const std::string& stage_name, bool success) {
        for (auto& p : plugins) p.instance->onStageEnd(stage_name, success);
    }

    bool tryExecutePluginStage(const std::string& stage_name, const JsonValue& config) {
        for (auto& p : plugins) {
            if (p.instance->executeCustomStage(stage_name, config)) {
                return true;
            }
        }
        return false;
    }

    void unloadAll() {
        for (auto& p : plugins) {
            delete p.instance;
            dlclose(p.handle);
        }
        plugins.clear();
    }
};
EOF

cat << 'EOF' > include/pipeline.hpp
#pragma once
#include "json_parser.hpp"
#include "plugin_manager.hpp"
#include <string>
#include <vector>

struct ProcessResult {
    int exit_code;
    std::string output;
};

class PipelineRunner {
    PluginManager plugin_manager;
    ProcessResult executeCommand(const std::vector<std::string>& args);

public:
    PipelineRunner();
    bool runBuildStage(const std::string& source, const std::string& output);
    bool runTestStage(const std::string& tests_dir);
    bool runDeployStage(const std::string& source, const std::string& target);
    bool runScriptStage(const std::string& script_path);
    bool runPipeline(const std::string& config_path);
};
EOF

cat << 'EOF' > src/pipeline.cpp
#include "pipeline.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <chrono>

namespace fs = std::filesystem;

PipelineRunner::PipelineRunner() {
    plugin_manager.loadPluginsFromDir("./plugins");
    plugin_manager.loadPluginsFromDir(".");
}

ProcessResult PipelineRunner::executeCommand(const std::vector<std::string>& args) {
    int pipefd[2];
    if (pipe(pipefd) == -1) return {-1, "Ошибка создания pipe"};

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return {-1, "Ошибка fork()"};
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        std::vector<char*> c_args;
        for (const auto& a : args) c_args.push_back(const_cast<char*>(a.c_str()));
        c_args.push_back(nullptr);

        execvp(c_args[0], c_args.data());
        _exit(127);
    }

    close(pipefd[1]);
    char buf[512];
    std::string out;
    ssize_t bytes;
    while ((bytes = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
        buf[bytes] = '\0';
        out += buf;
    }
    close(pipefd[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return {exit_code, out};
}

bool PipelineRunner::runBuildStage(const std::string& source, const std::string& output) {
    std::cout << "[BUILD] Компиляция исходных файлов из " << source << " в " << output << "...\n";
    if (!fs::exists(source)) {
        std::cerr << "[BUILD ERROR] Директория с исходным кодом не найдена: " << source << "\n";
        return false;
    }
    fs::create_directories(output);

    std::vector<std::string> cpp_files;
    for (const auto& entry : fs::directory_iterator(source)) {
        if (entry.path().extension() == ".cpp") {
            cpp_files.push_back(entry.path().string());
        }
    }

    if (cpp_files.empty()) {
        std::cerr << "[BUILD ERROR] Файлы .cpp не найдены в " << source << "\n";
        return false;
    }

    std::string out_app = output + "/app";
    std::vector<std::string> cmd = {"g++", "-std=c++17"};
    for (const auto& f : cpp_files) cmd.push_back(f);
    cmd.push_back("-o");
    cmd.push_back(out_app);

    auto res = executeCommand(cmd);
    if (res.exit_code != 0) {
        std::cerr << "[BUILD FAIL] Код возврата: " << res.exit_code << "\nВывод:\n" << res.output << "\n";
        return false;
    }
    std::cout << "[BUILD SUCCESS] Исполняемый файл создан: " << out_app << "\n";
    return true;
}

bool PipelineRunner::runTestStage(const std::string& tests_dir) {
    std::cout << "[TEST] Компиляция и запуск модульных тестов из " << tests_dir << "...\n";
    if (!fs::exists(tests_dir)) {
        std::cerr << "[TEST ERROR] Директория тестов не найдена: " << tests_dir << "\n";
        return false;
    }

    std::vector<std::string> test_files;
    for (const auto& entry : fs::directory_iterator(tests_dir)) {
        if (entry.path().extension() == ".cpp") {
            test_files.push_back(entry.path().string());
        }
    }

    if (test_files.empty()) {
        std::cerr << "[TEST ERROR] Файлы тестов не найдены в " << tests_dir << "\n";
        return false;
    }

    std::string test_bin = "./bin/test_runner";
    fs::create_directories("./bin");
    std::vector<std::string> compile_cmd = {"g++", "-std=c++17", "-Iinclude", "-pthread"};
    for (const auto& f : test_files) compile_cmd.push_back(f);
    compile_cmd.push_back("-o");
    compile_cmd.push_back(test_bin);

    auto comp_res = executeCommand(compile_cmd);
    if (comp_res.exit_code != 0) {
        std::cerr << "[TEST BUILD FAIL] Не удалось собрать тесты:\n" << comp_res.output << "\n";
        return false;
    }

    auto run_res = executeCommand({test_bin});
    std::cout << run_res.output;
    if (run_res.exit_code != 0) {
        std::cerr << "[TEST FAIL] Модульные тесты завершились с ошибками (Код: " << run_res.exit_code << ")\n";
        return false;
    }
    std::cout << "[TEST SUCCESS] Все тесты успешно пройдены!\n";
    return true;
}

bool PipelineRunner::runDeployStage(const std::string& source, const std::string& target) {
    std::cout << "[DEPLOY] Развертывание приложения из " << source << " в " << target << "...\n";
    std::string src_file = source + "/app";
    if (!fs::exists(src_file)) {
        std::cerr << "[DEPLOY ERROR] Файл для развертывания не найден: " << src_file << "\n";
        return false;
    }

    fs::create_directories(target);
    std::string target_file = target + "/app";
    try {
        fs::copy_file(src_file, target_file, fs::copy_options::overwrite_existing);
        chmod(target_file.c_str(), 0755);
    } catch (const std::exception& e) {
        std::cerr << "[DEPLOY ERROR] Ошибка копирования: " << e.what() << "\n";
        return false;
    }

    std::string report_path = target + "/deploy_report.txt";
    std::ofstream report(report_path);
    if (report.is_open()) {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        report << "=== ОТЧЕТ О РАЗВЕРТЫВАНИИ ===\n";
        report << "Дата и время: " << std::ctime(&now);
        report << "Исходный файл: " << src_file << "\n";
        report << "Назначение: " << target_file << "\n";
        report << "Статус: УСПЕШНО (без root/sudo в ALT Linux)\n";
        report.close();
    }

    std::cout << "[DEPLOY SUCCESS] Приложение развернуто в " << target_file << "\n";
    std::cout << "[DEPLOY REPORT] Сформирован отчет: " << report_path << "\n";
    return true;
}

bool PipelineRunner::runScriptStage(const std::string& script_path) {
    std::cout << "[SCRIPT] Выполнение Bash-скрипта: " << script_path << "...\n";
    if (!fs::exists(script_path)) {
        std::cerr << "[SCRIPT ERROR] Скрипт не найден: " << script_path << "\n";
        return false;
    }

    chmod(script_path.c_str(), 0755);

    auto res = executeCommand({script_path});
    std::cout << res.output;
    if (res.exit_code != 0) {
        std::cerr << "[SCRIPT FAIL] Скрипт вернул код ошибки: " << res.exit_code << "\n";
        return false;
    }
    return true;
}

bool PipelineRunner::runPipeline(const std::string& config_path) {
    std::cout << "=== ЗАПУСК ПАЙПЛАЙНА (" << config_path << ") ===\n";
    JsonValue config;
    try {
        config = JsonParser::parseFile(config_path);
    } catch (const std::exception& e) {
        std::cerr << "[CONFIG ERROR] " << e.what() << "\n";
        return false;
    }

    if (!config.has("pipeline") || !config["pipeline"].is_arr()) {
        std::cerr << "[CONFIG ERROR] Файл не содержит массива 'pipeline'\n";
        return false;
    }

    const auto& stages = config["pipeline"];
    for (size_t i = 0; i < stages.arr_val.size(); ++i) {
        const auto& st = stages.arr_val[i];
        std::string stage_name = st.has("stage") ? st["stage"].str_val : "unknown";

        plugin_manager.notifyStageStart(stage_name);
        bool success = false;

        if (stage_name == "build") {
            std::string src = st.has("source") ? st["source"].str_val : "./src";
            std::string out = st.has("output") ? st["output"].str_val : "./bin";
            success = runBuildStage(src, out);
        } else if (stage_name == "test") {
            std::string tdir = st.has("tests") ? st["tests"].str_val : "./tests";
            success = runTestStage(tdir);
        } else if (stage_name == "deploy") {
            std::string src = st.has("source") ? st["source"].str_val : "./bin";
            std::string tgt = st.has("target") ? st["target"].str_val : "./deploy";
            success = runDeployStage(src, tgt);
        } else if (stage_name == "script") {
            std::string sc = st.has("path") ? st["path"].str_val : "";
            success = runScriptStage(sc);
        } else {
            success = plugin_manager.tryExecutePluginStage(stage_name, st);
            if (!success) {
                std::cerr << "[PIPELINE ERROR] Неизвестный этап или ненайденный плагин: " << stage_name << "\n";
            }
        }

        plugin_manager.notifyStageEnd(stage_name, success);

        if (!success) {
            std::cerr << "\n[КРИТИЧЕСКАЯ ОШИБКА] Пайплайн остановлен на этапе '" << stage_name << "'.\n";
            return false;
        }
    }

    std::cout << "\n=== ПАЙПЛАЙН УСПЕШНО ЗАВЕРШЕН ===\n";
    return true;
}
EOF

cat << 'EOF' > src/main.cpp
#include "pipeline.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Использование:\n"
                  << "  ci-cd build --source <src_dir> --output <bin_dir>\n"
                  << "  ci-cd test --tests <tests_dir>\n"
                  << "  ci-cd deploy --source <bin_dir> --target <deploy_dir>\n"
                  << "  ci-cd run --config <pipeline.json>\n";
        return 1;
    }

    std::string command = argv[1];
    PipelineRunner runner;

    if (command == "build") {
        std::string source = "./src";
        std::string output = "./bin";
        for (int i = 2; i < argc; ++i) {
            if (std::string(argv[i]) == "--source" && i + 1 < argc) source = argv[++i];
            if (std::string(argv[i]) == "--output" && i + 1 < argc) output = argv[++i];
        }
        return runner.runBuildStage(source, output) ? 0 : 1;
    } else if (command == "test") {
        std::string tests = "./tests";
        for (int i = 2; i < argc; ++i) {
            if (std::string(argv[i]) == "--tests" && i + 1 < argc) tests = argv[++i];
        }
        return runner.runTestStage(tests) ? 0 : 1;
    } else if (command == "deploy") {
        std::string source = "./bin";
        std::string target = "./deploy";
        for (int i = 2; i < argc; ++i) {
            if (std::string(argv[i]) == "--source" && i + 1 < argc) source = argv[++i];
            if (std::string(argv[i]) == "--target" && i + 1 < argc) target = argv[++i];
        }
        return runner.runDeployStage(source, target) ? 0 : 1;
    } else if (command == "run") {
        std::string config = "pipeline.json";
        for (int i = 2; i < argc; ++i) {
            if (std::string(argv[i]) == "--config" && i + 1 < argc) config = argv[++i];
        }
        return runner.runPipeline(config) ? 0 : 1;
    } else {
        std::cerr << "Неизвестная команда: " << command << "\n";
        return 1;
    }
}
EOF

cat << 'EOF' > plugins/lint_plugin.cpp
#include "plugin_interface.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class LintPlugin : public PipelinePlugin {
public:
    std::string getName() const override { return "CodeLinterPlugin"; }

    bool executeCustomStage(const std::string& stage_name, const JsonValue& config) override {
        if (stage_name != "lint") return false;

        std::cout << "[LintPlugin] Старт статического анализа кода...\n";
        std::string source_dir = config.has("source") ? config["source"].str_val : "./src";

        if (!fs::exists(source_dir)) {
            std::cerr << "[LintPlugin ERROR] Директория не найдена: " << source_dir << "\n";
            return false;
        }

        int issues = 0;
        for (const auto& entry : fs::recursive_directory_iterator(source_dir)) {
            if (entry.is_regular_file() && (entry.path().extension() == ".cpp" || entry.path().extension() == ".hpp")) {
                std::ifstream file(entry.path());
                std::string line;
                int line_num = 0;
                while (std::getline(file, line)) {
                    line_num++;
                    if (line.length() > 120) {
                        std::cout << "  [LINT WARN] " << entry.path().filename().string() << ":" << line_num 
                                  << " Превышена длина строки (>120 символов)\n";
                        issues++;
                    }
                    if (line.find('\t') != std::string::npos) {
                        std::cout << "  [LINT WARN] " << entry.path().filename().string() << ":" << line_num 
                                  << " Найден символ табуляции '\\t'\n";
                        issues++;
                    }
                }
            }
        }

        std::cout << "[LintPlugin] Проверка завершена. Найдено замечаний: " << issues << "\n";
        return true;
    }
};

extern "C" PipelinePlugin* create_plugin() {
    return new LintPlugin();
}
EOF

cat << 'EOF' > plugins/notify_plugin.cpp
#include "plugin_interface.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

class NotifyPlugin : public PipelinePlugin {
    std::string getTimeStr() const {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&t), "%H:%M:%S");
        return ss.str();
    }

public:
    std::string getName() const override { return "NotificationPlugin"; }

    bool onStageStart(const std::string& stage_name) override {
        std::cout << ">>> [" << getTimeStr() << "] [УВЕДОМЛЕНИЕ] Запуск этапа: " << stage_name << "\n";
        return true;
    }

    bool onStageEnd(const std::string& stage_name, bool success) override {
        std::cout << "<<< [" << getTimeStr() << "] [УВЕДОМЛЕНИЕ] Этап " << stage_name 
                  << " завершен (" << (success ? "УСПЕШНО" : "СБОЙ") << ")\n";
        return true;
    }
};

extern "C" PipelinePlugin* create_plugin() {
    return new NotifyPlugin();
}
EOF

cat << 'EOF' > scripts/custom_step.sh
#!/bin/bash
echo "[CUSTOM SCRIPT] Проверка дискового пространства в рабочей директории:"
df -h .
EOF
chmod +x scripts/custom_step.sh

cat << 'EOF' > pipeline.json
{
  "pipeline": [
    { "stage": "lint", "source": "./sample_app/src" },
    { "stage": "build", "source": "./sample_app/src", "output": "./bin" },
    { "stage": "test", "tests": "./sample_app/tests" },
    { "stage": "script", "path": "./scripts/custom_step.sh" },
    { "stage": "deploy", "source": "./bin", "target": "./deploy" }
  ]
}
EOF

cat << 'EOF' > sample_app/src/main.cpp
#include <iostream>

int add(int a, int b) {
    return a + b;
}

int main() {
    std::cout << "Тестовое консольное приложение запущенное в ALT Linux!" << std::endl;
    std::cout << "2 + 2 = " << add(2, 2) << std::endl;
    return 0;
}
EOF

cat << 'EOF' > sample_app/tests/test_main.cpp
#include "test_framework.hpp"

int add(int a, int b) { return a + b; }

TEST(MathTests, AdditionTest) {
    ASSERT_EQ(add(2, 3), 5);
}

TEST(MathTests, FailCheck) {
    ASSERT_TRUE(add(10, 20) == 30);
}

TEST(StringTests, LengthTest) {
    std::string text = "ALTLinux";
    ASSERT_EQ(text.length(), 8);
}

int main() {
    return RunAllTestsParallel();
}
EOF

echo "Все файлы созданы!"