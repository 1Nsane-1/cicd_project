#include <iostream>
#include <unistd.h>
int main() {
    std::cout << "[Notify Service] Запущен (PID: " << getpid() << "). Сервис уведомлений активен.\n";
    return 0;
}