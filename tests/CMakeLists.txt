set(CMAKE_BUILD_TYPE Debug)
# set (CMAKE_CXX_STANDARD 17)

add_executable(
    unit_tests
    tengine.cpp
    tgameboard.cpp
    tutils.cpp
    tvalid_moves.cpp
)

target_include_directories(unit_tests 
    PUBLIC 
    ../src
)

target_link_libraries(
    unit_tests
    gtest_main
    ShumiChess
)