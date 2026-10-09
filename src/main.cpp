#include "pipeline.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Использование:\n"
                  << "  ci-cd build --source <src_dir> --output <bin_dir>\n"
                  << "  ci-cd test --tests <tests_dir>\n"
                  << "  ci-cd deploy --source <bin_dir> --target <deploy_dir>\n"
                  << "  ci-cd run --config <pipeline.json>\n";
        return 1;
    }

    std::string command = argv[1];
    PipelineRunner runner;

    if (command == "build") {
        std::string source = "./src";
        std::string output = "./bin";
        for (int i = 2; i < argc; ++i) {
            if (std::string(argv[i]) == "--source" && i + 1 < argc) source = argv[++i];
            if (std::string(argv[i]) == "--output" && i + 1 < argc) output = argv[++i];
        }
        return runner.runBuildStage(source, output) ? 0 : 1;
    } else if (command == "test") {
        std::string tests = "./tests";
        for (int i = 2; i < argc; ++i) {
            if (std::string(argv[i]) == "--tests" && i + 1 < argc) tests = argv[++i];
        }
        return runner.runTestStage(tests) ? 0 : 1;
    } else if (command == "deploy") {
        std::string source = "./bin";
        std::string target = "./deploy";
        for (int i = 2; i < argc; ++i) {
            if (std::string(argv[i]) == "--source" && i + 1 < argc) source = argv[++i];
            if (std::string(argv[i]) == "--target" && i + 1 < argc) target = argv[++i];
        }
        return runner.runDeployStage(source, target) ? 0 : 1;
    } else if (command == "run") {
        std::string config = "pipeline.json";
        for (int i = 2; i < argc; ++i) {
            if (std::string(argv[i]) == "--config" && i + 1 < argc) config = argv[++i];
        }
        return runner.runPipeline(config) ? 0 : 1;
    } else {
        std::cerr << "Неизвестная команда: " << command << "\n";
        return 1;
    }
}
