#include <iostream>
#include <unistd.h>
int main() {
    std::cout << "[User Service] Запущен (PID: " << getpid() << "). Сервис пользователей активен.\n";
    return 0;
}