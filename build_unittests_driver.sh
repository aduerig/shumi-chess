#!/bin/bash
cmake CMakeLists.txt && cmake --build . && ./bin/unit_tests && echo "STARTING DRIVER" && ./bin/ok
