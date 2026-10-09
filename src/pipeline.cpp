#include "pipeline.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <chrono>

namespace fs = std::filesystem;

PipelineRunner::PipelineRunner() {
    plugin_manager.loadPluginsFromDir("./plugins");
    plugin_manager.loadPluginsFromDir(".");
}

ProcessResult PipelineRunner::executeCommand(const std::vector<std::string>& args) {
    int pipefd[2];
    if (pipe(pipefd) == -1) return {-1, "Ошибка создания pipe"};

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return {-1, "Ошибка fork()"};
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        std::vector<char*> c_args;
        for (const auto& a : args) c_args.push_back(const_cast<char*>(a.c_str()));
        c_args.push_back(nullptr);

        execvp(c_args[0], c_args.data());
        _exit(127);
    }

    close(pipefd[1]);
    char buf[512];
    std::string out;
    ssize_t bytes;
    while ((bytes = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
        buf[bytes] = '\0';
        out += buf;
    }
    close(pipefd[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return {exit_code, out};
}

bool PipelineRunner::runBuildStage(const std::string& source, const std::string& output) {
    std::cout << "[BUILD] Компиляция исходных файлов из " << source << " в " << output << "...\n";
    if (!fs::exists(source)) {
        std::cerr << "[BUILD ERROR] Директория с исходным кодом не найдена: " << source << "\n";
        return false;
    }
    fs::create_directories(output);

    std::vector<std::string> cpp_files;
    for (const auto& entry : fs::directory_iterator(source)) {
        if (entry.path().extension() == ".cpp") {
            cpp_files.push_back(entry.path().string());
        }
    }

    if (cpp_files.empty()) {
        std::cerr << "[BUILD ERROR] Файлы .cpp не найдены в " << source << "\n";
        return false;
    }

    std::string out_app = output + "/app";
    std::vector<std::string> cmd = {"g++", "-std=c++17"};
    for (const auto& f : cpp_files) cmd.push_back(f);
    cmd.push_back("-o");
    cmd.push_back(out_app);

    auto res = executeCommand(cmd);
    if (res.exit_code != 0) {
        std::cerr << "[BUILD FAIL] Код возврата: " << res.exit_code << "\nВывод:\n" << res.output << "\n";
        return false;
    }
    std::cout << "[BUILD SUCCESS] Исполняемый файл создан: " << out_app << "\n";
    return true;
}

bool PipelineRunner::runTestStage(const std::string& tests_dir) {
    std::cout << "[TEST] Компиляция и запуск модульных тестов из " << tests_dir << "...\n";
    if (!fs::exists(tests_dir)) {
        std::cerr << "[TEST ERROR] Директория тестов не найдена: " << tests_dir << "\n";
        return false;
    }

    std::vector<std::string> test_files;
    for (const auto& entry : fs::directory_iterator(tests_dir)) {
        if (entry.path().extension() == ".cpp") {
            test_files.push_back(entry.path().string());
        }
    }

    if (test_files.empty()) {
        std::cerr << "[TEST ERROR] Файлы тестов не найдены в " << tests_dir << "\n";
        return false;
    }

    std::string test_bin = "./bin/test_runner";
    fs::create_directories("./bin");
    std::vector<std::string> compile_cmd = {"g++", "-std=c++17", "-Iinclude", "-pthread"};
    for (const auto& f : test_files) compile_cmd.push_back(f);
    compile_cmd.push_back("-o");
    compile_cmd.push_back(test_bin);

    auto comp_res = executeCommand(compile_cmd);
    if (comp_res.exit_code != 0) {
        std::cerr << "[TEST BUILD FAIL] Не удалось собрать тесты:\n" << comp_res.output << "\n";
        return false;
    }

    auto run_res = executeCommand({test_bin});
    std::cout << run_res.output;
    if (run_res.exit_code != 0) {
        std::cerr << "[TEST FAIL] Модульные тесты завершились с ошибками (Код: " << run_res.exit_code << ")\n";
        return false;
    }
    std::cout << "[TEST SUCCESS] Все тесты успешно пройдены!\n";
    return true;
}

bool PipelineRunner::runDeployStage(const std::string& source, const std::string& target) {
    std::cout << "[DEPLOY] Развертывание приложения из " << source << " в " << target << "...\n";
    std::string src_file = source + "/app";
    if (!fs::exists(src_file)) {
        std::cerr << "[DEPLOY ERROR] Файл для развертывания не найден: " << src_file << "\n";
        return false;
    }

    fs::create_directories(target);
    std::string target_file = target + "/app";
    try {
        fs::copy_file(src_file, target_file, fs::copy_options::overwrite_existing);
        chmod(target_file.c_str(), 0755);
    } catch (const std::exception& e) {
        std::cerr << "[DEPLOY ERROR] Ошибка копирования: " << e.what() << "\n";
        return false;
    }

    std::string report_path = target + "/deploy_report.txt";
    std::ofstream report(report_path);
    if (report.is_open()) {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        report << "=== ОТЧЕТ О РАЗВЕРТЫВАНИИ ===\n";
        report << "Дата и время: " << std::ctime(&now);
        report << "Исходный файл: " << src_file << "\n";
        report << "Назначение: " << target_file << "\n";
        report << "Статус: УСПЕШНО (без root/sudo в ALT Linux)\n";
        report.close();
    }

    std::cout << "[DEPLOY SUCCESS] Приложение развернуто в " << target_file << "\n";
    std::cout << "[DEPLOY REPORT] Сформирован отчет: " << report_path << "\n";
    return true;
}

bool PipelineRunner::runScriptStage(const std::string& script_path) {
    std::cout << "[SCRIPT] Выполнение Bash-скрипта: " << script_path << "...\n";
    if (!fs::exists(script_path)) {
        std::cerr << "[SCRIPT ERROR] Скрипт не найден: " << script_path << "\n";
        return false;
    }

    chmod(script_path.c_str(), 0755);

    auto res = executeCommand({script_path});
    std::cout << res.output;
    if (res.exit_code != 0) {
        std::cerr << "[SCRIPT FAIL] Скрипт вернул код ошибки: " << res.exit_code << "\n";
        return false;
    }
    return true;
}

bool PipelineRunner::runPipeline(const std::string& config_path) {
    std::cout << "=== ЗАПУСК ПАЙПЛАЙНА (" << config_path << ") ===\n";
    JsonValue config;
    try {
        config = JsonParser::parseFile(config_path);
    } catch (const std::exception& e) {
        std::cerr << "[CONFIG ERROR] " << e.what() << "\n";
        return false;
    }

    if (!config.has("pipeline") || !config["pipeline"].is_arr()) {
        std::cerr << "[CONFIG ERROR] Файл не содержит массива 'pipeline'\n";
        return false;
    }

    const auto& stages = config["pipeline"];
    for (size_t i = 0; i < stages.arr_val.size(); ++i) {
        const auto& st = stages.arr_val[i];
        std::string stage_name = st.has("stage") ? st["stage"].str_val : "unknown";

        plugin_manager.notifyStageStart(stage_name);
        bool success = false;

        if (stage_name == "build") {
            std::string src = st.has("source") ? st["source"].str_val : "./src";
            std::string out = st.has("output") ? st["output"].str_val : "./bin";
            success = runBuildStage(src, out);
        } else if (stage_name == "test") {
            std::string tdir = st.has("tests") ? st["tests"].str_val : "./tests";
            success = runTestStage(tdir);
        } else if (stage_name == "deploy") {
            std::string src = st.has("source") ? st["source"].str_val : "./bin";
            std::string tgt = st.has("target") ? st["target"].str_val : "./deploy";
            success = runDeployStage(src, tgt);
        } else if (stage_name == "script") {
            std::string sc = st.has("path") ? st["path"].str_val : "";
            success = runScriptStage(sc);
        } else {
            success = plugin_manager.tryExecutePluginStage(stage_name, st);
            if (!success) {
                std::cerr << "[PIPELINE ERROR] Неизвестный этап или ненайденный плагин: " << stage_name << "\n";
            }
        }

        plugin_manager.notifyStageEnd(stage_name, success);

        if (!success) {
            std::cerr << "\n[КРИТИЧЕСКАЯ ОШИБКА] Пайплайн остановлен на этапе '" << stage_name << "'.\n";
            return false;
        }
    }

    std::cout << "\n=== ПАЙПЛАЙН УСПЕШНО ЗАВЕРШЕН ===\n";
    return true;
}
