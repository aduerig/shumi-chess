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
cd driver
python3 build_c_module_for_python.py build --force
ret_code=$?
cd ..
if [ $ret_code -eq 0 ]; then
    python3 driver/show_board.py
fi