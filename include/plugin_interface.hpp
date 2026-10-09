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
