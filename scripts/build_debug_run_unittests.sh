#!/bin/bash

# -Wno-dev: supresses the deprecated warnings
# -DBUILD_TESTS: controls if tests are built
cmake CMakeLists.txt -Wno-dev -DBUILD_TESTS=ON && cmake --build . && ./bin/unit_tests --gtest_color=yes && echo "RUNNING EXECUTABLE" && ./bin/ok
