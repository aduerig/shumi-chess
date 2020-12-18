cmake_minimum_required(VERSION 3.19)

set(this ShumiChess)

project(${this})

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

enable_testing()
add_subdirectory(gtests)

set(Headers
    engine.hpp
)

set(Sources
    engine.cpp
)

add_library(${this} STATIC ${Sources} ${Headers})