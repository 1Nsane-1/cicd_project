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
