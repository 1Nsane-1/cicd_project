cmake_minimum_required(VERSION 3.12)
project(cicd_project CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(ci-cd
    src/main.cpp
    src/json_parser.cpp
    src/pipeline.cpp
)
target_link_libraries(ci-cd PRIVATE ${CMAKE_DL_LIBS} pthread)

add_library(lint SHARED plugins/lint_plugin.cpp)
set_target_properties(lint PROPERTIES PREFIX "lib")

add_library(notify SHARED plugins/notify_plugin.cpp)
set_target_properties(notify PROPERTIES PREFIX "lib")
