cmake_minimum_required(VERSION 3.24)
project(Project3 C)

set(CMAKE_C_STANDARD 11)
find_package(Threads REQUIRED)

#add_executable(Project3 main.c)
add_executable(Project3 deneme.c)

target_link_libraries(Project3 Threads::Threads)

