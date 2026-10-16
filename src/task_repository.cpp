#include "json_parser.cpp"
#include <vector>
#include <algorithm>
#include <iostream>

class TaskRepository {
private:
    std::string filepath = "data/tasks.json";
    std::vector<Task> tasks;

public:
    TaskRepository() {
        tasks = JsonParser::parseTasks(filepath);
    }

    void addTask(const std::string& title) {
        int newId = tasks.empty() ? 1 : tasks.back().id + 1;
        tasks.push_back({newId, title, false});
        JsonParser::saveTasks(filepath, tasks);
        std::cout << "Задача успешно добавлена (ID: " << newId << ")\n";
    }

    void deleteTask(int id) {
        auto it = std::remove_if(tasks.begin(), tasks.end(), [id](const Task& t) {
            return t.id == id;
        });
        if (it != tasks.end()) {
            tasks.erase(it, tasks.end());
            JsonParser::saveTasks(filepath, tasks);
            std::cout << "Задача с ID " << id << " удалена.\n";
        } else {
            std::cout << "Ошибка: Задача с ID " << id << " не найдена.\n";
        }
    }

    void listTasks() {
        tasks = JsonParser::parseTasks(filepath);
        if (tasks.empty()) {
            std::cout << "Список задач пуст.\n";
            return;
        }
        std::cout << "\n--- СПИСОК ЗАДАЧ ---\n";
        for (const auto& t : tasks) {
            std::cout << "[" << (t.completed ? "X" : " ") << "] ID: " << t.id << " - " << t.title << "\n";
        }
    }

    void completeTask(int id) {
        bool found = false;
        for (auto& t : tasks) {
            if (t.id == id) {
                t.completed = true;
                found = true;
                break;
            }
        }
        if (found) {
            JsonParser::saveTasks(filepath, tasks);
            std::cout << "Задача " << id << " отмечена как выполненная.\n";
        } else {
            std::cout << "Ошибка: Задача с ID " << id << " не найдена.\n";
        }
    }
};