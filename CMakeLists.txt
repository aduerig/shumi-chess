cmake_minimum_required(VERSION 3.15)

set(project_name ShumiChess)
project(${project_name} CXX)
set(CMAKE_CXX_STANDARD 17)

# --- Set a default build type if none is specified (for single-config generators) ---
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as default.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Linux detected. Forcing debug symbols with the -g flag.")
    add_compile_options(-g)
endif()

# --- Set output directories using Generator Expressions for multi-config safety ---
# $<CONFIG> is evaluated at build time to the current configuration (e.g., Debug, Release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)


# --- Define project sources and headers ---
set(Headers
    src/gameboard.hpp
    src/engine.hpp
    src/globals.hpp
    src/move_tables.hpp
    src/utility.hpp
    src/minimax.hpp
)

set(Sources
    src/gameboard.cpp
    src/globals.cpp
    src/engine.cpp
    src/move_tables.cpp
    src/utility.cpp
    src/minimax.cpp
)


if(BUILD_TESTS)
    include(googletest_cmake/googletest.cmake)
    fetch_googletest(
        ${PROJECT_SOURCE_DIR}/googletest_cmake
        ${PROJECT_BINARY_DIR}/googletest
    )
    add_subdirectory(tests)
endif(BUILD_TESTS)

add_library(${project_name} STATIC ${Sources} ${Headers})

set_property(TARGET ${project_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(${project_name} PUBLIC src)


add_executable(measure_speed_random_games driver/measure_speed_random_games.cpp)
target_link_libraries(measure_speed_random_games PUBLIC ${project_name})

add_executable(run_minimax_time src/run_minimax_time.cpp)
target_link_libraries(run_minimax_time PUBLIC ${project_name})

add_executable(shumi_driver driver/driver.cpp)
target_link_libraries(shumi_driver PUBLIC ${project_name})
