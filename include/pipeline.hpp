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
