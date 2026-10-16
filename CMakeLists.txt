cmake_minimum_required(VERSION 3.12)
project(TaskManager CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Основные директории
include_directories(${CMAKE_SOURCE_DIR}/src)

# Главное приложение
add_executable(task-manager
    src/main.cpp
    src/json_parser.cpp
    src/task_repository.cpp
    src/architecture_monolith.cpp
    src/architecture_client_server.cpp
    src/architecture_microservices.cpp
    src/use_case_generator.cpp
    src/plugin_loader.cpp
    src/logger.cpp
)
# Потоки и dlopen (dl) для плагинов
target_link_libraries(task-manager dl pthread)

# Микросервисы (отдельные исполняемые файлы)
add_executable(task_service services/task_service.cpp)
add_executable(user_service services/user_service.cpp)
add_executable(notify_service services/notify_service.cpp)

# Плагины (.so библиотеки)
add_library(rule_scrum SHARED plugins/rule_scrum.cpp)
set_target_properties(rule_scrum PROPERTIES PREFIX "lib" SUFFIX ".so")

add_library(rule_waterfall SHARED plugins/rule_waterfall.cpp)
set_target_properties(rule_waterfall PROPERTIES PREFIX "lib" SUFFIX ".so")