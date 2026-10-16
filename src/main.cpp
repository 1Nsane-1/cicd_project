#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>

// --- Структуры и Логгер ---
struct Task {
    int id;
    std::string title;
    bool completed;
};

class Logger {
public:
    static void log(const std::string& message) {
        system("mkdir -p logs");
        std::ofstream logFile("logs/app.log", std::ios_base::app);
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        if (logFile.is_open()) {
            logFile << "[" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << "] " << message << "\n";
        }
        std::cout << "[LOG] " << message << std::endl;
    }
};

// --- Репозиторий задач ---
class TaskRepository {
private:
    std::string filepath = "data/tasks.json";
    std::vector<Task> tasks;

public:
    TaskRepository() {
        // Загрузка (упрощенная)
        system("mkdir -p data");
        std::ifstream file(filepath);
        if (!file.is_open()) {
            tasks = { {1, "Изучить архитектуру ОС ALT Linux", true}, {2, "Спроектировать ТРПО Task Manager", false} };
            saveTasks();
            return;
        }
        tasks = { {1, "Изучить архитектуру ОС ALT Linux", true}, {2, "Спроектировать ТРПО Task Manager", false} };
    }

    void saveTasks() {
        std::ofstream file(filepath);
        file << "[\n";
        for (size_t i = 0; i < tasks.size(); ++i) {
            file << "  {\"id\": " << tasks[i].id << ", \"title\": \"" << tasks[i].title << "\", \"completed\": " << (tasks[i].completed ? "true" : "false") << "}\n";
        }
        file << "]\n";
    }

    void listTasks() {
        std::cout << "\n--- СПИСОК ЗАДАЧ ---\n";
        for (const auto& t : tasks) {
            std::cout << "[" << (t.completed ? "X" : " ") << "] ID: " << t.id << " - " << t.title << "\n";
        }
    }
};

// --- Генератор Use Case диаграмм ---
class UseCaseGenerator {
public:
    static void generateAndSave(const std::string& archName) {
        std::string content = "Use Case Diagram (" + archName + "):\n\n"
                              "  Actor: User\n"
                              "    - Add Task\n"
                              "    - Delete Task\n"
                              "    - View Tasks\n"
                              "    - Mark Task as Completed\n\n"
                              "  System: Task Manager\n";
        std::cout << "\n" << content;
        system("mkdir -p output");
        std::ofstream outfile("output/use-case-diagram.md");
        outfile << "# UML Use Case Diagram\n\n```text\n" << content << "```\n";
        std::cout << "Диаграмма сохранена в файл: output/use-case-diagram.md\n";
    }
};

// --- Архитектуры ---
void runMonolithInfo() {
    std::cout << "\nАрхитектура: Монолитная\nПреимущества: Простота разработки, единая кодовая база.\nНедостатки: Сложность масштабирования.\n";
}

void runClientServerInfo() {
    std::cout << "\nАрхитектура: Клиент-серверная\nПреимущества: Разделение UI и логики.\nНедостатки: Зависимость от сети.\n";
}

void runMicroservicesInfo(const std::string& config) {
    std::cout << "\nАрхитектура: Микросервисная (конфиг: " << config << ")\nПреимущества: Независимое масштабирование.\nНедостатки: Сложность IPC.\n";
    mkfifo("data/task_fifo", 0666);
}

// --- Главная точка входа ---
int main(int argc, char* argv[]) {
    Logger::log("Запуск приложения Task Manager");

    if (argc < 3) {
        std::cout << "Использование: task-manager --architecture [monolith|client-server|microservices]\n";
        TaskRepository repo;
        repo.listTasks();
        return 0;
    }

    std::string arg1 = argv[1];
    std::string arch = argv[2];
    std::string archName = "Monolithic";

    if (arg1 == "--architecture") {
        if (arch == "monolith") {
            runMonolithInfo();
            archName = "Monolithic";
        } else if (arch == "client-server") {
            runClientServerInfo();
            archName = "Client-Server";
        } else if (arch == "microservices") {
            std::string config = (argc > 4) ? argv[4] : "data/services.json";
            runMicroservicesInfo(config);
            archName = "Microservices";
        } else {
            std::cerr << "Неизвестная архитектура.\n";
            return 1;
        }

        UseCaseGenerator::generateAndSave(archName);
        TaskRepository repo;
        repo.listTasks();
    }

    Logger::log("Работа завершена.");
    return 0;
}