# 趣味
we're back

## Quickstart
### Dependancies
* Python
* IF WINDOWS: 
  * [mingw](https://github.com/niXman/mingw-builds-binaries)
  * lld: error: unable to find library -lvcruntime140 ([i think you need to install the x86 and x64 from here](https://learn.microsoft.com/en-US/cpp/windows/latest-supported-vc-redist?view=msvc-170))

### How to run gui
* Linux
  * `.\scripts\build_debug_run_gui.sh`
* Windows
  * `.\scripts\win_build_debug_run_gui.ps1`


## intro
This project is a hobby C++ engine written by OhMesch and I. It functions as a fast library exposing useful chess functions such as `generate_legal_moves`. A python module `engine_communicator` is also avaliable. There is a serviceable python gui to see the engine in action at `driver/show_board.py`.

This project has no AI to make intelligent chess moves. There is another repo written by OhMesch and I that consumes this library to write both a Minimax based AI and a Montecarlo AI based on Deepmind's AlphaZero:
* https://github.com/aduerig/chess-ai


## building
This project uses CMAKE for C++ parts of the engine. It also exposes a python module written in C++ that can access the board state, and simple engine commands for the purposes of a GUI.

* use the files in the `scripts/` folder

* building the C++ python module
  * **IMPORTANT: Tkinter must be installed on your system!** 
    * For arch linux: `sudo pacman -S tk`)
  * `./build_python_module_run.sh`

* build, execute tests, and run driver (Linux)
  * `./build_unittests_driver.sh`

* Run before compilation to compile with clang
```
export CXX=/usr/bin/clang++
export CC=/usr/bin/clang
```

* build, execute tests, and run driver if using mingw (you are probably on windows) (https://github.com/google/googletest/issues/1051)
  * `cmake CMakeLists.txt -Dgtest_disable_pthreads=ON && cmake --build . && .\bin\unit_tests.exe`

## testing
This project uses GTest for its unittests, after building with `cmake --build .` the unit_test binary will be in the bin/ folder

## benchmarking
`perf stat -e L1-icache-loads,L1-icache-load-misses,L1-dcache-loads,L1-dcache-load-misses ./bin/measure_speed_random_games`

## developing
* if you are facing "unresolved imports" on vscode in the `show_board.py` python file, you can create the file `.env` and add as the contents
** `PYTHONPATH=driver`

## technical notes
### using this as CMAKE + gtest template
https://github.com/bast/gtest-demo

### using cpython to compile C extensions to communicate to c++ engine for gui
https://docs.python.org/3/extending/

### bit twiddling hacks
http://graphics.stanford.edu/%7Eseander/bithacks.html

### for gtest, maybe want to use this
https://gist.github.com/elliotchance/8215283


## profiling
* perf (by function)
  * perf record -o ./bin/perf.data -g ./bin/run_minimax_depth $1
  * perf report -g  -i ./bin/perf.data
* valgrind (line by line hopefully)
  * valgrind --tool=callgrind ./bin/run_minimax_depth 5
  * kcachegrind callgrind.out.pid
* gprof (line by line)
  * add `-pg` to complilation, and -l
  * `gprof -l ./my_program > analysis.txt`
