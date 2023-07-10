cmake CMakeLists.txt -Wno-dev -DCMAKE_BUILD_TYPE=Release && cmake --build .
# ret_code=$?
# if [ $ret_code -ne 0 ]; then
#     echo "cmake CMakeLists.txt FAILED"
#     exit
# fi
# cmake --build .
# ret_code=$?
# if [ $ret_code -ne 0 ]; then
#     echo "cmake --build . FAILED"
#     exit
# fi

python3 driver/build_c_module_for_python.py build --build-lib="driver" --build-temp="driver/build_temp" --force --steps_to_root 1

ret_code=$?
if [ $ret_code -eq 0 ]; then    
    echo "successfully built engine_communicator module"
    python3 driver/build_c_module_for_minimax.py build --build-lib="driver" --build-temp="driver/build_temp" --force

    ret_code=$?
    if [ $ret_code -eq 0 ]; then
        echo "successfully built minimax module"
        python driver/show_board.py    
    else
        echo "FAILED running: 'python3 driver/build_c_module_for_minimax.py'"
    fi

else
    echo "FAILED running: 'python3 driver/build_c_module_for_python.py'"
fi


# cd src
# python3 build_c_module_for_minimax.py build --force
# ret_code=$?
# cd ..
# if [ $ret_code -eq 0 ]; then
#     python3 driver/show_board.py
# fi