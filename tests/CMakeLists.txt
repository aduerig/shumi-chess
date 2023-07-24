if(WIN32)
    # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    if(${CMAKE_BUILD_TYPE} MATCHES DEBUG)
        message("IN TEST CMAKE DEBUG DETECTED")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug$<$<CONFIG:Debug>:Debug>")
        add_compile_options(/MTd) # release is /MD
    endif()
endif()


# set(CMAKE_BUILD_TYPE Debug)
# set (CMAKE_CXX_STANDARD 17)

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

target_link_libraries(
    unit_tests
    PUBLIC
    gtest_main
    ShumiChess
)