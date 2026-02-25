
#include <math.h>

#include <chrono>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <sstream>

#ifdef SHUMI_FORCE_ASSERTS  // Operated by the -asserts" and "-no-asserts" args to run_gui.py. By default on.
#undef NDEBUG
#endif
#include <assert.h>

#include <engine.hpp>
#include <globals.hpp>
#include <utility.hpp>

#include "minimax.hpp"

using namespace std;
using namespace ShumiChess;
using namespace std::chrono;

////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {

    assert(0);       // To insure that asserts are compiled out

    string FENString =  "r2qnrk1/1p2ppbp/p5p1/2p1N3/b1B5/1PN5/1B1P1PPP/R1R1Q1K1 w - - 0 14";
    Engine engine(FENString);

    string out = utility::representation::gameboard_to_string(engine.game_board);
    cout << out << endl;

    //Engine engine;


    MinimaxAI minimax_ai(engine);

    int time_to_use = 1000;
    int depth_to_use = 10;
    if (argc < 2) {
        //cout << "You entered no argument for 'time_to_use', using default value of " << time_to_use << "msec" << endl;
    } else {
        time_to_use = atoi(argv[1]);
        //cout << "You entered time_to_use of: " << time_to_use << endl;
    }
    cout << "using level= " << depth_to_use << "  msec = " << time_to_use << endl;

    // minimax_ai.get_move(time_to_use);
    minimax_ai.get_move_iterative_deepening(time_to_use, depth_to_use, 0);
    cout << "Got a move at time_to_use: " << time_to_use << endl;
    return 0;
}

//      run: cmake --build C:\programming\shumi-chess\build --config RelWithDebInfo
//      look in: C:\programming\shumi-chess\build\bin\RelWithDebInfo

