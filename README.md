# 趣味
we're back

## intro
This project is a hobby C++ engine written by OhMesch and I. It functions as a fast library exposing useful chess functions such as `generate_legal_moves`. A python module `engine_communicator` is also avaliable. There is a serviceable python gui to see the engine in action at `driver/show_board.py`.

This project has no AI to make intelligent chess moves. There is another repo written by OhMesch and I that consumes this library to write both a Minimax based AI and a Montecarlo AI based on Deepmind's AlphaZero:
* https://github.com/aduerig/chess-ai


## building
This project uses CMAKE for C++ parts of the engine. It also exposes a python module written in C++ that can access the board state, and simple engine commands for the purposes of a GUI.

* building the C++ engine
  * `cmake CMakeLists.txt`

* building the C++ python module
  * **IMPORTANT: Tkinter must be installed on your system!** 
    * For arch linux: `sudo pacman -S tk`)
  * `./build_python_module_run.sh`

* build, execute tests, and run driver (Linux)
  * `./build_unittests_driver.sh`

* build, execute tests, and run driver if using mingw (you are probably on windows) (https://github.com/google/googletest/issues/1051)
  * `cmake CMakeLists.txt -Dgtest_disable_pthreads=ON && cmake --build . && .\bin\unit_tests.exe`

* 1-liner to build python module and run it
  * `python build_c_module_for_python.py build; python show_board.py`

run to compile with clang (might be faster) 
```
export CXX=/usr/bin/clang++
export CC=/usr/bin/clang
```


## testing
This project uses GTest for its unittests, after building with `cmake --build .` the unit_test binary will be in the bin/ folder

## benchmarking
`perf stat -e L1-icache-loads,L1-icache-load-misses,L1-dcache-loads,L1-dcache-load-misses ./bin/measure_speed_random_games`

## technical notes
### using this as CMAKE + gtest template
https://github.com/bast/gtest-demo

### using cpython to compile C extensions to communicate to c++ engine for gui
https://docs.python.org/3/extending/

### bit twiddling hacks
http://graphics.stanford.edu/%7Eseander/bithacks.html
