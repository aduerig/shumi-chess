cmake CMakeLists.txt -Wno-dev -DBUILD_TESTS=ON -Dgtest_disable_pthreads=ON 
if ( $LASTEXITCODE -ne 0 ) {
    echo "cmake CMakeLists.txt FAILED"
    exit
}
cmake --build .
if ( $LASTEXITCODE -ne 0 ) {
    echo "cmake --build . FAILED"
    exit
}
./bin/unit_tests --gtest_color=yes