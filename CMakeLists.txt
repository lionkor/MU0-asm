cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)

include_directories(String/include)
link_directories(String)

project(mu0asm)

add_executable(${CMAKE_PROJECT_NAME} main.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME} String)
