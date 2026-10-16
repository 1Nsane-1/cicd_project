#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>

class Logger {
public:
    static void log(const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::string logDir = "logs";
        // Создаем папку logs, если нет
        system("mkdir -p logs");
        
        std::ofstream logFile("logs/app.log", std::ios_base::app);
        if (logFile.is_open()) {
            logFile << "[" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << "] " 
                    << message << "\n";
        }
        std::cout << "[LOG] " << message << std::endl;
    }
};