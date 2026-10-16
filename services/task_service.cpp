#include <iostream>
#include <unistd.h>
int main() {
    std::cout << "[Task Service] Запущен (PID: " << getpid() << "). Ожидание задач через FIFO...\n";
    sleep(1); // Эмуляция работы сервиса
    return 0;
}