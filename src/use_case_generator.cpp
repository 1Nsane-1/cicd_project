#include <iostream>
#include <fstream>
#include <string>

class UseCaseGenerator {
public:
    static void generateAndSave(const std::string& architectureName) {
        std::string content = "Use Case Diagram (" + architectureName + "):\n\n"
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