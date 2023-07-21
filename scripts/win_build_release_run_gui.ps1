cmake CMakeLists.txt -Wno-dev -DCMAKE_BUILD_TYPE=Release
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

if ( $LASTEXITCODE -ne 0 ) {
    echo 'FAILED running: "python build_c_module_for_python.py build -c mingw32 --force"'
    exit
}

python driver/show_board.py
