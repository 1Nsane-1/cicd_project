#ifndef PIPELINE_H
#define PIPELINE_H

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <windows.h>
#include "json.h"
#include "process_runner.h"
#include "plugin_interface.h"

namespace fs = std::filesystem;

class PipelineManager {
    std::mutex log_mutex;
    std::string current_logs;
    std::string current_status = "IDLE";

public:
    void log(const std::string& msg) {
        std::lock_guard<std::mutex> lock(log_mutex);
        current_logs += msg + "\n";
    }

    std::string get_logs() {
        std::lock_guard<std::mutex> lock(log_mutex);
        return current_logs;
    }

    std::string get_status() {
        return current_status;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(log_mutex);
        current_logs.clear();
        current_status = "RUNNING";
    }

    bool execute_build(const std::string& source_dir, const std::string& output_dir) {
        log("[BUILD] Compiling C++ source files...");
        fs::create_directories(output_dir);
        std::string cmd = "g++ -std=c++17 " + source_dir + "/*.cpp -o " + output_dir + "/app.exe";
        std::string out;
        bool ok = run_command(cmd, out);
        log(out);
        if (!ok) log("[BUILD] Failed!");
        else log("[BUILD] Success: " + output_dir + "/app.exe created.");
        return ok;
    }

    bool execute_test() {
        log("[TEST] Running unit tests...");
        std::string out;
        bool ok = run_command("g++ -std=c++17 ./tests/sample_test.cpp -o ./bin/tests.exe && ./bin/tests.exe", out);
        log(out);
        return ok;
    }

    bool execute_deploy(const std::string& src_dir, const std::string& target_dir) {
        log("[DEPLOY] Deploying artifacts...");
        try {
            fs::create_directories(target_dir);
            if (fs::exists(src_dir)) {
                fs::copy(src_dir, target_dir, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            }
            if (fs::exists("./web")) {
                fs::copy("./web", target_dir + "/web", fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            }
            log("[DEPLOY] Deployment completed successfully to " + target_dir);
            return true;
        } catch (const std::exception& e) {
            log(std::string("[DEPLOY] Error: ") + e.what());
            return false;
        }
    }

    bool execute_plugin(const std::string& dll_path) {
        log("[PLUGIN] Loading dynamic plugin: " + dll_path);
        HMODULE hModule = LoadLibraryA(dll_path.c_str());
        if (!hModule) {
            log("[PLUGIN] Failed to load DLL: " + dll_path);
            return false;
        }

        auto create_fn = (CreatePluginFn)GetProcAddress(hModule, "CreatePlugin");
        if (!create_fn) {
            log("[PLUGIN] Symbol 'CreatePlugin' not found!");
            FreeLibrary(hModule);
            return false;
        }

        PipelinePlugin* plugin = create_fn();
        log(std::string("[PLUGIN] Executing plugin: ") + plugin->name());

        PipelineContext ctx;
        ctx.run_id = "run_1";
        ctx.work_dir = fs::current_path().string();
        ctx.log_fn = [](const char* m) { std::cout << m << std::endl; };

        bool result = plugin->execute(&ctx);
        delete plugin;
        FreeLibrary(hModule);
        return result;
    }

    bool run_pipeline_file(const std::string& config_path) {
        reset();
        std::ifstream f(config_path);
        if (!f.is_open()) {
            log("[ERROR] Cannot open config file: " + config_path);
            current_status = "FAILED";
            return false;
        }

        std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        JsonParser parser(str);
        JsonValue root = parser.parse_value();

        if (root.obj_val.find("pipeline") == root.obj_val.end()) {
            log("[ERROR] Invalid JSON config: 'pipeline' key missing.");
            current_status = "FAILED";
            return false;
        }

        auto steps = root.obj_val["pipeline"].arr_val;
        for (auto& step : steps) {
            std::string stage = step.obj_val["stage"].str_val;
            if (stage == "build") {
                std::string src = step.obj_val.count("source") ? step.obj_val["source"].str_val : "./src";
                std::string out = step.obj_val.count("output") ? step.obj_val["output"].str_val : "./bin";
                if (!execute_build(src, out)) { current_status = "FAILED"; return false; }
            } else if (stage == "test") {
                if (!execute_test()) { current_status = "FAILED"; return false; }
            } else if (stage == "deploy") {
                std::string src = step.obj_val.count("source") ? step.obj_val["source"].str_val : "./bin";
                std::string target = step.obj_val.count("target") ? step.obj_val["target"].str_val : "./deploy";
                if (!execute_deploy(src, target)) { current_status = "FAILED"; return false; }
            } else if (stage == "plugin") {
                std::string path = step.obj_val["path"].str_val;
                if (!execute_plugin(path)) { current_status = "FAILED"; return false; }
            }
        }

        current_status = "SUCCESS";
        log("[PIPELINE] Completed successfully!");
        return true;
    }
};

#endif