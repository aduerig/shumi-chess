#!/bin/bash
cmake CMakeLists.txt && cmake --build . && ./bin/unit_tests --gtest_color=yes && echo "STARTING DRIVER" && ./bin/ok
