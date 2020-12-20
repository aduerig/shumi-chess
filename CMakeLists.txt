cmake_minimum_required(VERSION 3.19)

set(project_name ShumiChess)

# project(${project_name})

# adding in case want differnt name and folder
set(LIBRARY_NAME ${project_name})   # [optional] generated automatically (in lowercase)
set(LIBRARY_FOLDER src) # [optional] generated automatically (in lowercase)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

enable_testing()
add_subdirectory(gtests)

set(Headers
    src/Gameboard.hpp
    src/engine.hpp
    src/typedef.hpp
)

set(Sources
    src/Gameboard.hpp
    src/engine.cpp
)

# makes the libShumiChess.a file
add_library(${project_name} STATIC ${Sources} ${Headers})

# makes it so others can reference the source files for #include statements
target_include_directories(${project_name} PUBLIC src)

# makes the driver executable
add_executable(ok driver/driver.cpp)
target_link_libraries(ok PUBLIC ${project_name})


# message( ${CMAKE_CURRENT_SOURCE_DIR} )

# target_include_directories(ShumiChess PUBLIC ${CMAKE_CURRENT_LIST_DIR}/ShumiChess)

# add_executable(ShumiChess engine.cpp)