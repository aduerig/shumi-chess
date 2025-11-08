#include <cstdio>
#include <ostream>
#include <iostream>
#include <cstring>

#include <globals.hpp>
#include <engine.hpp>
#include <minimax.hpp>
#include <utility.hpp>

using namespace std;
using namespace ShumiChess;

#undef NDEBUG
//#define NDEBUG         // Define (uncomment) this to disable asserts
#include <assert.h>

int main(int argc, char** argv)
{
    // defaults
    double time_to_use_msec = 1000.0;   // -t1000
    int    depth_limit      = 5;        // -d5
    string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";  // -f

    // parse only attached forms: -t8  -d6  -f<fen>
    for (int i = 1; i < argc; ++i)
    {
        // -t<number>
        if (std::strncmp(argv[i], "-t", 2) == 0 && argv[i][2] != '\0')
        {
            time_to_use_msec = std::atof(argv[i] + 2);
        }
        // -d<number>
        else if (std::strncmp(argv[i], "-d", 2) == 0 && argv[i][2] != '\0')
        {
            depth_limit = std::atoi(argv[i] + 2);
        }
        // -f<string>
        else if (std::strncmp(argv[i], "-f", 2) == 0 && argv[i][2] != '\0')
        {
            fen = argv[i] + 2;
        }
        else
        {
            // optional: print bad arg
            std::cout << "Unrecognized arg: " << argv[i] << "\n";
        }
    }

    // make engine from FEN
    Engine engine(fen);

    // make AI
    MinimaxAI minimax_ai(engine);

    // show what we parsed
    cout << endl;
    cout << "time = " << time_to_use_msec << " msec\n";
    cout << "depth = " << depth_limit << "\n";
    cout << "fen = " << fen << "\n";
    cout << endl;

    Move move_best;

    move_best = minimax_ai.get_move_iterative_deepening(time_to_use_msec, depth_limit);


    engine.move_into_string(move_best);
    cout << "best= " << engine.move_string << endl;



    // your test
    assert(0);

    return 0;
}
