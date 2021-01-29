# causes errors, should probaly figure out why
# cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE DEBUG)
endif()

# variable set of the whole library/project
set(project_name ShumiChess)

project(${project_name} CXX)
set (CMAKE_CXX_STANDARD 17)


# cmdline to not build tests
# -DBUILD_TESTS=ON       or       -DBUILD_TESTS=OFF
option(BUILD_TESTS "This is settable from the command line" OFF)
message("BUILDING TESTS ${BUILD_TESTS}")


# these are if we dont call project(), it can seperate the library name and folder
# set(LIBRARY_NAME ${project_name})   # [optional] generated automatically (in lowercase)
# set(LIBRARY_FOLDER src) # [optional] generated automatically (in lowercase)

# place binaries and libraries according to GNU standards
include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

# we use this to get code coverage
# if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
# endif()

# https://stackoverflow.com/questions/566472/where-are-the-gcov-symbols
# https://stackoverflow.com/questions/11783932/how-do-i-add-a-linker-or-compile-flag-in-a-cmake-file
# SET(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage -fprofile-generate")
# SET(GCC_COVERAGE_LINK_FLAGS    "-lgcov --coverage")
# SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
# SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

add_subdirectory(src)

if(BUILD_TESTS)
    include(cmake/googletest.cmake)
    fetch_googletest(
        ${PROJECT_SOURCE_DIR}/cmake
        ${PROJECT_BINARY_DIR}/googletest
        )

    # creates Testing directory, commenting out
    # enable_testing()
    add_subdirectory(tests)
endif(BUILD_TESTS)


set(Headers
    src/gameboard.hpp
    src/engine.hpp
    src/globals.hpp
    src/move_tables.hpp
    src/utility.hpp
)

set(Sources
    src/gameboard.cpp
    src/globals.cpp
    src/engine.cpp
    src/move_tables.cpp
    src/utility.cpp
)

add_library(${project_name} STATIC ${Sources} ${Headers})
# add_library(${project_name} SHARED ${Sources} ${Headers})

# equivelent of '-fPIC' as a compiler flag in gcc. only looks needed if not compiling for shared
set_property(TARGET ${project_name} PROPERTY POSITION_INDEPENDENT_CODE ON)


# # makes it so others can reference the source files for #include statements
target_include_directories(${project_name} PUBLIC src)

set(CMAKE_CXX_FLAGS_DEBUG "-g")
# -fno-omit-frame-pointer does have a perf hit, but it enables call stacks when using perf -g
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -fno-omit-frame-pointer")
# set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# all the fast flags (Ofast helps but hardly)!
# set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -ffloat-store -ffast-math -fno-rounding-math -fno-signaling-nans -fcx-limited-range -fno-math-errno -funsafe-math-optimizations -fassociative-math -freciprocal-math -ffinite-math-only -fno-signed-zeros -fno-trapping-math -frounding-math -fsingle-precision-constant -fcx-fortran-rules")

# dont think we need, was a strange error when adding a minimum version  
# target_link_libraries(${project_name} PUBLIC pthread)

add_executable(measure_speed_random_games driver/measure_speed_random_games.cpp)
target_link_libraries(measure_speed_random_games PUBLIC ${project_name})

# unsets the variable to clear it from the cache
unset(BUILD_TESTS CACHE)