#include <iostream>
#include "../src/plugin_loader.cpp"

class ScrumRule : public ArchitectureRule {
public:
    void applyRule() override {
        std::cout << "[PLUGIN RULE] Применено правило Scrum: Фокус на итеративной разработке спринтами.\n";
    }
};

extern "C" ArchitectureRule* create_rule() {
    return new ScrumRule();
}