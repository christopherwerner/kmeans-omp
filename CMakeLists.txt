cmake_minimum_required(VERSION 3.17)
project(project1 C)
set(CMAKE_C_COMPILER "/usr/local/bin/gcc-10")
set(CMAKE_C_STANDARD 99)

#message(STATUS "CMAKE FLAGS: ${CMAKE_C_FLAGS}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
#add_definitions(-DDEBUG)
add_executable(kmeans kmeans.c csvhelper.c csvhelper.h kmeans.h)
