cmake CMakeLists.txt -Wno-dev
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
python3 driver/build_c_module_for_python.py build --force --build-lib="driver" --build-temp="driver/build_temp" --steps_to_root 1
ret_code=$?
if [ $ret_code -eq 0 ]; then
    python3 driver/show_board.py
fi