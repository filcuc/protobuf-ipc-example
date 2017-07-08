cmake_minimum_required(VERSION 3.8)

project(client)
find_package(Qt5Core REQUIRED)
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} Qt5::Core)
