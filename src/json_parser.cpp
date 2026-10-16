#include <string>
#include <vector>
#include <fstream>
#include <sstream>

struct Task {
    int id;
    std::string title;
    bool completed;
};

class JsonParser {
public:
    static std::vector<Task> parseTasks(const std::string& filepath) {
        std::vector<Task> tasks;
        std::ifstream file(filepath);
        if (!file.is_open()) return tasks;

        std::string line;
        Task currentTask{0, "", false};
        bool inTask = false;

        while (std::getline(file, line)) {
            if (line.find("{") != std::string::npos) {
                inTask = true;
                currentTask = {0, "", false};
            }
            if (inTask) {
                if (line.find("\"id\"") != std::string::npos) {
                    size_t colon = line.find(":");
                    currentTask.id = std::stoi(line.substr(colon + 1));
                }
                if (line.find("\"title\"") != std::string::npos) {
                    size_t first = line.find("\"");
                    size_t second = line.rfind("\"");
                    // Упрощенный поиск строки
                    size_t start = line.find(":", first);
                    size_t q1 = line.find("\"", start);
                    size_t q2 = line.rfind("\"");
                    currentTask.title = line.substr(q1 + 1, q2 - q1 - 1);
                }
                if (line.find("\"completed\"") != std::string::npos) {
                    currentTask.completed = (line.find("true") != std::string::npos);
                }
            }
            if (line.find("}") != std::string::npos && inTask) {
                tasks.push_back(currentTask);
                inTask = false;
            }
        }
        return tasks;
    }

    static void saveTasks(const std::string& filepath, const std::vector<Task>& tasks) {
        system("mkdir -p data");
        std::ofstream file(filepath);
        file << "[\n";
        for (size_t i = 0; i < tasks.size(); ++i) {
            file << "  {\n";
            file << "    \"id\": " << tasks[i].id << ",\n";
            file << "    \"title\": \"" << tasks[i].title << "\",\n";
            file << "    \"completed\": " << (tasks[i].completed ? "true" : "false") << "\n";
            file << "  }" << (i + 1 < tasks.size() ? "," : "") << "\n";
        }
        file << "]\n";
    }
};