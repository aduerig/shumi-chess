cmake CMakeLists.txt
if ( $LASTEXITCODE -ne 0 ) {
    echo "cmake CMakeLists.txt FAILED"
    exit
}
cmake --build .
if ( $LASTEXITCODE -ne 0 ] {
    echo "cmake CMakeLists.txt FAILED"
    exit
}
cd driver
python build_c_module_for_python.py --force build -c mingw32
cd ..
if ( $LASTEXITCODE -eq 0 ) {
    python driver/show_board.py
}