#!/bin/bash
cmake CMakeLists.txt -Wno-dev -DCMAKE_BUILD_TYPE=Release

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

# -g measures callgraphs
perf record -o ./bin/perf.data -g ./bin/run_minimax_depth $1
ret_code=$?
if [ $ret_code -ne 0 ]; then
    echo "FAILED: 'perf record -g ./bin/run_minimax_depth $1'"
    exit
fi


perf report -g  -i ./bin/perf.data

# to sort properly 
# perf report -g  -i ./bin/perf.data --no-children

# 32 minutes into https://www.youtube.com/watch?v=nXaxk27zwlk
# .5 is a filter
# caller inverts the graph
# perf report -g 'graph,0.5,caller