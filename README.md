# 趣味
we're back

## building
This project uses CMAKE for C++ parts of the engine. It also exposes a python module written in C++ that can access the board state, and simple engine commands for the purposes of a GUI.

### building the C++ engine
`cmake CMakeLists.txt`

build, execute tests, and run driver (Linux)

`./build_unittests_driver.sh`

build, execute tests, and run driver if using mingw (probably windows) (https://github.com/google/googletest/issues/1051)

`cmake CMakeLists.txt -Dgtest_disable_pthreads=ON && cmake --build . && .\bin\unit_tests.exe`

### building the C++ python module
Tkinter must be installed on your system. For arch linux: `sudo pacman -S tk`
`./build_python_module_run.sh`

#### 1-liner to build python module and run it
`python build_c_module_for_python.py build; python show_board.py`

## testing
This project uses GTest for its unittests, after building the unit_test binary will be in the bin/ folder

## technical notes
### using this as CMAKE + gtest template
https://github.com/bast/gtest-demo

### using cpython to compile C extensions to communicate to c++ engine for gui
https://docs.python.org/3/extending/

### bit twiddling hacks
http://graphics.stanford.edu/%7Eseander/bithacks.html
