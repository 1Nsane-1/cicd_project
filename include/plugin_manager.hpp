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
