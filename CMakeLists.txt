# -DBUILD_TESTS=ON       or       -DBUILD_TESTS=OFF
# set(BUILD_TESTS OFF)
# option(BUILD_TESTS "This is settable from the command line" OFF CACHE BOOL "idk" FORCE)

# !TODO THIS DOESNT WORK DUE TO CACHE STUFF
option(BUILD_TESTS "blank" OFF)
message("BUILDING TESTS ${BUILD_TESTS}")

set(project_name ShumiChess)
project(${project_name} CXX)
set (CMAKE_CXX_STANDARD 17)


# how to do this?
# set(CMAKE_C_COMPILER gcc)
# set(CMAKE_CXX_COMPILER g++)


if(NOT CMAKE_BUILD_TYPE)
    message("NO BUILD TYPE DETECTED, DEFAULTING TO DEBUG")
    set(CMAKE_BUILD_TYPE DEBUG)
endif()

message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")


if(${CMAKE_BUILD_TYPE} MATCHES DEBUG)
    # -fno-omit-frame-pointer does have a perf hit, but it enables call stacks when using perf -g
    add_compile_options(-g -fno-omit-frame-pointer)
else()
    # all the fast flags (Ofast helps but hardly)!
    # set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -ffloat-store -ffast-math -fno-rounding-math -fno-signaling-nans -fcx-limited-range -fno-math-errno -funsafe-math-optimizations -fassociative-math -freciprocal-math -ffinite-math-only -fno-signed-zeros -fno-trapping-math -frounding-math -fsingle-precision-constant -fcx-fortran-rules")
    # maybe we don't always want -g, maybe pass in only for perf stuff
    # add_compile_options(-Ofast -march=native -ffloat-store -ffast-math -fno-rounding-math -fno-signaling-nans -fcx-limited-range -fno-math-errno -funsafe-math-optimizations -fassociative-math -freciprocal-math -ffinite-math-only -fno-signed-zeros -fno-trapping-math -frounding-math -fsingle-precision-constant -fcx-fortran-rules)
    add_compile_options(-g -Ofast)
endif()


include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)


message("CMAKE_ARCHIVE_OUTPUT_DIRECTORY: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
message("CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}")

add_subdirectory(src)

if(BUILD_TESTS)
    include(googletest_cmake/googletest.cmake)
    fetch_googletest(
        ${PROJECT_SOURCE_DIR}/googletest_cmake
        ${PROJECT_BINARY_DIR}/googletest
    )
    add_subdirectory(tests)
endif(BUILD_TESTS)


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

# add_library(${project_name} SHARED ${Sources} ${Headers})
add_library(${project_name} STATIC ${Sources} ${Headers})

# equivelent of '-fPIC' as a compiler flag in gcc. only looks needed if not compiling for shared
set_property(TARGET ${project_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

# makes it so others can reference the source files for #include statements
target_include_directories(${project_name} PUBLIC src)


add_executable(measure_speed_random_games driver/measure_speed_random_games.cpp)
add_executable(run_minimax_time src/run_minimax_time.cpp)
target_link_libraries(run_minimax_time PUBLIC ${project_name})
target_link_libraries(measure_speed_random_games PUBLIC ${project_name})


unset(BUILD_TESTS)
unset(CMAKE_BUILD_TYPE)





# cmake_minimum_required(VERSION 3.5 FATAL_ERROR) # errors, prob figure out why

# if(MSVC)
#     message("BUILDING WITH MSVC DETECTED")
#     set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
# else()
#     message("NOT BUILDING WITH MSVC")
# endif()



# you can seperate out project() to folder name and project name somehow

# https://stackoverflow.com/questions/566472/where-are-the-gcov-symbols
# https://stackoverflow.com/questions/11783932/how-do-i-add-a-linker-or-compile-flag-in-a-cmake-file
# SET(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage -fprofile-generate")
# SET(GCC_COVERAGE_LINK_FLAGS    "-lgcov --coverage")
# SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
# SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")



# stuff for msvc


# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# if(WIN32)
# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug$<$<CONFIG:Debug>:Debug>")
# add_compile_options(/MTd) # release is /MD
# else()
# # add_compile_options("/Ox /RTCu")


# if(WIN32)
#     # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#     if(${CMAKE_BUILD_TYPE} MATCHES DEBUG)
#         message("IN TEST CMAKE DEBUG DETECTED")
#         set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug$<$<CONFIG:Debug>:Debug>")
#         add_compile_options(/MTd) # release is /MD
#     endif()
# endif()


# # set(CMAKE_BUILD_TYPE Debug)
# # set (CMAKE_CXX_STANDARD 17)

# if(${CMAKE_BUILD_TYPE} MATCHES DEBUG)
#     set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# else()
#     set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)    
# endif()