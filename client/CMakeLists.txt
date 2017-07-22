cmake_minimum_required(VERSION 3.8)

project(client)

find_package(Protobuf REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(PROTO_DIR ../server/protos)
set(PROJECT_SRCS main.cpp application.cpp socket.cpp global.cpp)
set(PROJECT_HDRS application.h socket.h global.h)
set(PROTO_ARGN "${PROTO_DIR}/message.proto")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_ARGN})
add_executable(${PROJECT_NAME} ${PROJECT_SRCS} ${PROJECT_HDRS} ${PROTO_SRCS} ${PROTO_HDRS})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11)
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Network ${PROTOBUF_LIBRARIES})
