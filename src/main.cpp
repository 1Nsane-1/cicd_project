#include <iostream>
#include <string>
#include "task_repository.cpp"
#include "architecture_monolith.cpp"
#include "architecture_client_server.cpp"
#include "architecture_microservices.cpp"
#include "use_case_generator.cpp"
#include "plugin_loader.cpp"
#include "logger.cpp"

void printHelp() {
    std::cout << "Использование: task-manager --architecture [monolith|client-server|microservices] [--start config.json]\n";
}

int main(int argc, char* argv[]) {
    Logger::log("Запуск приложения Task Manager");

    if (argc < 3) {
        printHelp();
        // Интерактивный режим по умолчанию для демонстрации
        TaskRepository repo;
        repo.listTasks();
        return 0;
    }

    std::string arg1 = argv[1];
    std::string arch = argv[2];

    if (arg1 == "--architecture") {
        if (arch == "monolith") {
            runMonolithInfo();
            UseCaseGenerator::generateAndSave("Monolithic");
        } else if (arch == "client-server") {
            runClientServerInfo();
            UseCaseGenerator::generateAndSave("Client-Server");
        } else if (arch == "microservices") {
            std::string config = "data/services.json";
            if (argc > 4 && std::string(argv[3]) == "--start") {
                config = argv[4];
            }
            runMicroservicesInfo(config);
            UseCaseGenerator::generateAndSave("Microservices");
        } else {
            std::cerr << "Ошибка: Неизвестная архитектура '" << arch << "'.\n";
            return 1;
        }
    }

    // Пример теста плагина, если он есть
    // PluginLoader::loadPlugin("./librulo_scrum.so");

    Logger::log("Работа приложения завершена успешно.");
    return 0;
}