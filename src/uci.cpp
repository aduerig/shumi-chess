
#include <math.h>

#include <chrono>
#include <conio.h>
#include <cstdio>
#include <iostream>
#include <limits>
#include <ostream>
#include <sstream>
#include <thread>

///////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
    std::string line;

    while (std::getline(std::cin, line)) {

        if (line == "uci") {
            std::cout << "id name ShumiChess\n";
            std::cout << "id author Paul Duerig\n";
            std::cout << "uciok\n";
            std::cout.flush();

        } else if (line == "isready") {
            std::cout << "readyok\n";
            std::cout.flush();

        } else if (line == "ucinewgame") {
            // clear TT, repetition table, history, etc.

        } else if (line.rfind("position ", 0) == 0) {
            // set board from "startpos" or "fen"
            // then play the listed moves

        } else if (line.rfind("go ", 0) == 0) {
            // call your existing Shumi search
            // get best move
            std::cout << "bestmove e2e4\n";   // replace with real move
            std::cout.flush();

        } else if (line == "quit") {
            break;
        }
    }

    return 0;
}