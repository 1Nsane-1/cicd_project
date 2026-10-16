#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

void runMicroservicesInfo(const std::string& jsonConfig) {
    std::cout << "\nВыбранная архитектура: микросервисная (конфиг: " << jsonConfig << ")\n";
    std::cout << "Преимущества:\n  - Независимое масштабирование сервисов (Задачи, Пользователи, Уведомления).\n  - Изоляция сбоев.\n";
    std::cout << "Недостатки:\n  - Сложная маршрутизация и межпроцессное взаимодействие.\n";

    // Создаем FIFO канал для демонстрации POSIX IPC
    mkfifo("data/task_fifo", 0666);
    
    std::cout << "[POSIX] Запуск фоновых сервисов через fork + execvp...\n";
    pid_t pid = fork();
    if (pid == 0) {
        // Дочерний процесс (эмуляция микросервиса)
        execlp("./task_service", "task_service", NULL);
        _exit(1);
    }
}