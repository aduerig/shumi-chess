
#include <math.h>

#include <chrono>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <sstream>

#undef NDEBUG
// #define NDEBUG         // Define (uncomment) this to disable asserts
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
    Engine engine;
    MinimaxAI minimax_ai(engine);

    double time_to_use = 30;
    if (argc < 2) {
        cout << "You entered no argument for 'time_to_use', using default value of 1" << endl;
    } else {
        time_to_use = atoi(argv[1]);
        cout << "You entered time_to_use of: " << time_to_use << endl;
    }
    // minimax_ai.get_move(time_to_use);
    minimax_ai.get_move_iterative_deepening(time_to_use, 7);
    cout << "Got a move at time_to_use: " << time_to_use << endl;
    return 0;
}