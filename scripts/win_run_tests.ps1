cmake CMakeLists.txt -Wno-dev -DBUILD_TESTS=ON -Dgtest_disable_pthreads=ON -DCMAKE_BUILD_TYPE=Release
if ( $LASTEXITCODE -ne 0 ) {
    echo "cmake CMakeLists.txt FAILED"
    exit
}
cmake --build . --config Debug
if ( $LASTEXITCODE -ne 0 ) {
    echo "cmake --build . FAILED"
    exit
}
./bin/unit_tests --gtest_color=yes