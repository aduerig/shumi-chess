# 趣味
we're back

## building
This project uses CMAKE for C++ parts of the engine. It also exposes a python module written in C++ that can access the board state, and simple engine commands for the purposes of a GUI.

### building the C++ engine
`cmake CMakeLists.txt`

#### 1-liner to build, execute tests, and run driver
`cmake CMakeLists.txt; cmake --build .; ./bin/unit_tests; echo "STARTING DRIVER"; ./bin/ok`

### building the C++ python module
`cd driver`
`python build_c_module_for_python.py build`

#### 1-liner to build python module and run it
`python build_c_module_for_python.py build; python show_board.py`

## testing
This project uses GTest for its unittests, after building the unit_test binary will be in the bin/ folder

## in depth technical notes
### using this as CMAKE + gtest template
https://github.com/bast/gtest-demo

### using cpython to compile C extensions to communicate to c++ engine for gui
https://docs.python.org/3/extending/