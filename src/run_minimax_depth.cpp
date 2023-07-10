
#include <cstdio>
#include <ostream>
#include <iostream>
#include <chrono> 
#include <math.h>
#include <sstream>

#include <globals.hpp>
#include <engine.hpp>
#include <utility.hpp>

#include "minimax.hpp"

using namespace std;
using namespace ShumiChess;
using namespace std::chrono;


int main(int argc, char** argv) {
    Engine engine;
    MinimaxAI minimax_ai(engine);

    int depth = 1;
    if (argc < 2) {
        cout << "You entered no argument for 'depth', using default value of 1" << endl;
    }
    else {
        depth = atoi(argv[1]);
        cout << "You entered depth of: " << depth << endl;
    }
    minimax_ai.get_move(depth);
    cout << "Got a move at depth: " << depth << endl;
    return 0;
}