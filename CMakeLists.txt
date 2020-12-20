cmake_minimum_required(VERSION 3.19)

set(this ShumiChess)

project(${this})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

enable_testing()
add_subdirectory(gtests)

set(Headers
    Board.hpp
    engine.hpp
    typedef.hpp
)

set(Sources
    Board.hpp
    engine.cpp
)

add_library(${this} STATIC ${Sources} ${Headers})