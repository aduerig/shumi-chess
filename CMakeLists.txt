# causes errors, should probaly figure out why
# cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# variable set of the whole library/project
set(project_name ShumiChess)
set(CMAKE_BUILD_TYPE Debug)

project(${project_name} CXX)
set (CMAKE_CXX_STANDARD 17)

# these are if we dont call project(), it can seperate the library name and folder
# set(LIBRARY_NAME ${project_name})   # [optional] generated automatically (in lowercase)
# set(LIBRARY_FOLDER src) # [optional] generated automatically (in lowercase)

# place binaries and libraries according to GNU standards
include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

# we use this to get code coverage
if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

add_subdirectory(src)

include(cmake/googletest.cmake)
fetch_googletest(
    ${PROJECT_SOURCE_DIR}/cmake
    ${PROJECT_BINARY_DIR}/googletest
    )

# creates Testing directory, commenting out
# enable_testing()
add_subdirectory(tests)

set(Headers
    src/gameboard.hpp
    src/engine.hpp
    src/globals.hpp
    src/utility.hpp
)

set(Sources
    src/gameboard.cpp
    src/engine.cpp
    src/utility.cpp
)

add_library(${project_name} STATIC ${Sources} ${Headers})

# # makes it so others can reference the source files for #include statements
target_include_directories(${project_name} PUBLIC src)

# dont think we need, was a strange error when adding a minimum version  
# target_link_libraries(${project_name} PUBLIC pthread)

# # makes the driver executable
add_executable(ok driver/driver.cpp)
target_link_libraries(ok PUBLIC ${project_name})