#include <iostream>
#include <dlfcn.h>

struct ArchitectureRule {
    virtual void applyRule() = 0;
    virtual ~ArchitectureRule() {}
};

typedef ArchitectureRule* (*CreateRuleFunc)();

class PluginLoader {
public:
    static void loadPlugin(const std::string& path) {
        void* handle = dlopen(path.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "Ошибка загрузки плагина: " << dlerror() << "\n";
            return;
        }
        dlerror(); // Очистка ошибок
        CreateRuleFunc createRule = (CreateRuleFunc)dlsym(handle, "create_rule");
        const char* dlsym_error = dlerror();
        if (dlsym_error) {
            std::cerr << "Ошибка поиска символа create_rule: " << dlsym_error << "\n";
            dlclose(handle);
            return;
        }
        ArchitectureRule* rule = createRule();
        std::cout << "[PLUGIN] Успешно загружен плагин из " << path << "\n";
        rule->applyRule();
        delete rule;
        dlclose(handle);
    }
};