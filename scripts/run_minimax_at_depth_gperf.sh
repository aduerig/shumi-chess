#!/bin/bash
cmake CMakeLists.txt -Wno-dev -DCMAKE_BUILD_TYPE=Release -DGPERF=ON

ret_code=$?
if [ $ret_code -ne 0 ]; then
    echo "FAILED: 'cmake CMakeLists.txt'"
    exit
fi

cmake --build .
ret_code=$?
if [ $ret_code -ne 0 ]; then
    echo "FAILED: 'cmake --build .'"
    exit
fi

gprof -l ./bin/run_minimax_depth $1 > gperf_analysis.txt
echo "look in gperf_analysis.txt"