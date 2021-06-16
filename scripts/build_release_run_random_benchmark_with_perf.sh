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
perf record -g ./bin/measure_speed_random_games
ret_code=$?
if [ $ret_code -ne 0 ]; then
    echo "FAILED: 'perf record ./bin/measure_speed_random_games'"
    exit
fi

perf report -g

# 32 minutes into https://www.youtube.com/watch?v=nXaxk27zwlk
# .5 is a filter
# caller inverts the graph
# perf report -g 'graph,0.5,caller'
