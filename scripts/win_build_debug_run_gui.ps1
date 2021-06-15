# probably really broken, check out .sh files for argument changes

cmake CMakeLists.txt -Wno-dev
if ( $LASTEXITCODE -ne 0 ) {
    echo "cmake CMakeLists.txt FAILED"
    exit
}
cmake --build .
if ( $LASTEXITCODE -ne 0 ) {
    echo "cmake --build ."
    exit
}
cd driver
python build_c_module_for_python.py build -c mingw32 --force
cd ..
if ( $LASTEXITCODE -eq 0 ) {
    python driver/show_board.py
}