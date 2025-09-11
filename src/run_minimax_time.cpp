
#include <cstdio>
#include <ostream>
#include <iostream>
#include <chrono> 
#include <math.h>
#include <sstream>

//#define NDEBUG         // Define (uncomment) this to disable asserts
#undef NDEBUG
#include <assert.h>

#include <globals.hpp>
#include <engine.hpp>
#include <utility.hpp>

#include "minimax.hpp"

using namespace std;
using namespace ShumiChess;
using namespace std::chrono;

////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    
    // Create the chess engine
    Engine engine;

    assert(0);   // NOTE: I dont get here?
    
    // Create the AI player (minnimax)
    MinimaxAI minimax_ai(engine);

    double time_to_use = 1;
    if (argc < 2) {
        cout << "You entered no argument for 'time_to_use', using default value of 1" << endl;
    }
    else {
        time_to_use = atoi(argv[1]);
        cout << "You entered time_to_use of: " << time_to_use << endl;
    }
    // minimax_ai.get_move(time_to_use);
    minimax_ai.get_move_iterative_deepening(time_to_use);
    cout << "Got a move at time_to_use: " << time_to_use << endl;
    return 0;
}