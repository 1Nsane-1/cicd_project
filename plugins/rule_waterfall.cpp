#include <iostream>
#include "../src/plugin_loader.cpp"

class WaterfallRule : public ArchitectureRule {
public:
    void applyRule() override {
        std::cout << "[PLUGIN RULE] Применено правило Waterfall: Строго последовательные этапы разработки.\n";
    }
};

extern "C" ArchitectureRule* create_rule() {
    return new WaterfallRule();
}