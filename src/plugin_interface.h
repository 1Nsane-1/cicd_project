#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <string>

struct PipelineContext {
    std::string run_id;
    std::string work_dir;
    void (*log_fn)(const char* msg);
};

class PipelinePlugin {
public:
    virtual ~PipelinePlugin() = default;
    virtual const char* name() const = 0;
    virtual bool execute(PipelineContext* ctx) = 0;
};

typedef PipelinePlugin* (*CreatePluginFn)();

#endif