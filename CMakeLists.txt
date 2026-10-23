cmake_minimum_required(VERSION 3.16)
project(lightbar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(lightbar src/main.cpp)

target_include_directories(lightbar PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(lightbar PRIVATE ${OpenCV_LIBS})