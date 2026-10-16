cmake_minimum_required(VERSION 3.12)
project(TaskManager CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(task-manager src/main.cpp)
target_link_libraries(task-manager dl pthread)