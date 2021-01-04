cd driver
python build_c_module_for_python.py build
ret_code=$?
echo $ret_code
cd ..
if [ $ret_code -eq 0 ]; then
    python driver/show_board.py
fi