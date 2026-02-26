add_executable(
    unit_tests
    tengine.cpp
    tgameboard.cpp
    tutils.cpp
    tvalid_moves.cpp
)

# set_target_properties(unit_tests PROPERTIES
#     MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
# )

target_include_directories(unit_tests
    PUBLIC
    ../src
)

target_compile_definitions(unit_tests PRIVATE
    TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/test_data/"
)

target_link_libraries(
    unit_tests
    PUBLIC
    gtest_main
    ShumiChess
)