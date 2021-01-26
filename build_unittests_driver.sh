#!/bin/bash
cmake CMakeLists.txt -Wno-dev -DBUILD_TESTS=ON && cmake --build . && ./bin/unit_tests --gtest_color=yes && echo "STARTING DRIVER" && ./bin/ok
