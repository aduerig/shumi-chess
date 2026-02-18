
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

    assert(0);      //To ensure no asserts are on here in this build.

    string FENString =  "r2qnrk1/1p2ppbp/p5p1/2p1N3/b1B5/1PN5/1B1P1PPP/R1R1Q1K1 w - - 0 14";
    Engine engine(FENString);

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

//
// 1. is_square_in_check() is top time waster.
// 2. in_check_after_move_fast() is third highest on the list of time wasters.
// 3. is_square_in_check() is called only by is_king_in_check2().
// 4. is_king_in_check2() is called in : 
//   4a. the in_check_after_move(), sandwiched between popMoveFast() and pushMoveFast():
//      pushMoveFast(move);   
//      bool bReturn = is_king_in_check2(color);
//      popMoveFast();
//
//   4b. in_check_after_move_fast(), sandwiched in a similar way:
//        ... code imitating a fast push but only doing what is needed "It is strictly a fast "after-move is king attacked?" query."
//        const bool bReturn = is_king_in_check2(color);
//        ... doing imiating a pop (of the above push)
//
//
// 5. in_check_after_move_fast() is called from get_legal_moves() (also high on the time waster list). lIke so:
//      get_psuedo_legal_moves(color, psuedo_legal_moves);
//      for (const Move& move : psuedo_legal_moves) {
//          bool bKingInCheck = in_check_after_move_fast(color, move);
//          if (!bKingInCheck) {
//              all_legal_moves.emplace_back(move);
//          }
//      }
//
// 6. get_psuedo_legal_moves() is not a big time waster at all. It gets all legal moves, assumming they dont
//    put the king in check.
//
