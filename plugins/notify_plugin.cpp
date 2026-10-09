#include "plugin_interface.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

class NotifyPlugin : public PipelinePlugin {
    std::string getTimeStr() const {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&t), "%H:%M:%S");
        return ss.str();
    }

public:
    std::string getName() const override { return "NotificationPlugin"; }

    bool onStageStart(const std::string& stage_name) override {
        std::cout << ">>> [" << getTimeStr() << "] [УВЕДОМЛЕНИЕ] Запуск этапа: " << stage_name << "\n";
        return true;
    }

    bool onStageEnd(const std::string& stage_name, bool success) override {
        std::cout << "<<< [" << getTimeStr() << "] [УВЕДОМЛЕНИЕ] Этап " << stage_name 
                  << " завершен (" << (success ? "УСПЕШНО" : "СБОЙ") << ")\n";
        return true;
    }
};

extern "C" PipelinePlugin* create_plugin() {
    return new NotifyPlugin();
}
