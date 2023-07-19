cmake CMakeLists.txt -Wno-dev -DCMAKE_BUILD_TYPE=Release
if ( $LASTEXITCODE -ne 0 ) {
    echo "FAILED running: cmake CMakeLists.txt FAILED"
    exit
}
cmake --build .
if ( $LASTEXITCODE -ne 0 ) {
    echo "FAILED running: cmake --build ."
    exit
}
python driver/build_c_module_for_python.py build -c mingw32 --build-lib="driver" --build-temp="driver/build_temp" --force --steps_to_root 1
if ( $LASTEXITCODE -ne 0 ) {
    echo "FAILED running: 'python driver/build_c_module_for_python.py'"
    exit
}
python driver/show_board.py