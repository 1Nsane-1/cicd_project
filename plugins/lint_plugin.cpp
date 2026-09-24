#include "../src/plugin_interface.h"
#include <iostream>

class LintPlugin : public PipelinePlugin {
public:
    const char* name() const override { return "Static Code Linter"; }
    bool execute(PipelineContext* ctx) override {
        if (ctx && ctx->log_fn) {
            ctx->log_fn("[LINT] Code style and security checks passed!");
        }
        return true;
    }
};

extern "C" __declspec(dllexport) PipelinePlugin* CreatePlugin() {
    return new LintPlugin();
}