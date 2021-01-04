# set(CMAKE_BUILD_TYPE Debug)
# set (CMAKE_CXX_STANDARD 17)

add_executable(
    unit_tests
    tgameboard.cpp
    tutils.cpp
    tvalid_moves.cpp
)

target_include_directories(unit_tests 
    PUBLIC 
    ../src
    # src
)

target_link_libraries(
    unit_tests
    gtest_main
    # /home/andrew/programming/shumi-chess/lib/libShumiChess.a
    ShumiChess
)
    

# add_test(
#   NAME
#     unit
#   COMMAND
#     ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/unit_tests
#   )