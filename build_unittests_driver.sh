#!/usr/bin/env
cmake CMakeLists.txt && cmake --build . && ./bin/unit_tests && echo "STARTING DRIVER" && ./bin/ok
