cmake CMakeLists.txt -Wno-dev -DCMAKE_BUILD_TYPE=Release
ret_code=$?
if [ $ret_code -ne 0 ]; then
    echo "cmake CMakeLists.txt FAILED"
    exit
fi
cmake --build .
ret_code=$?
if [ $ret_code -ne 0 ]; then
    echo "cmake --build . FAILED"
    exit
fi

./bin/measure_speed_random_games