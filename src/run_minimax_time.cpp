
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
    assert(0);
    Engine engine;
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

// 1. is_square_in_check2() is top time waster, called only by is_king_in_check().  
//    in_check_after_move_fast() is also very high on list of time wasters. 
// 2. is_king_in_check() is called in : a. the old in_check_after_move(), sandwiched between popMoveFast() and pushMoveFast().
//    also  is_king_in_check() is used in  b. in_check_after_move_fast(), sandwiched in a similar way.


//
// My Move structure is 56 bytes, too large, as I can represent a move in algebriac text as 8 characters (8 bytes) at most. 
// But the two big things in this move struture is these ulls for .from and .to. These are guarenteed bitboards, guarenteed to 
// have only one bit set. From here on, in this document, when I say bitboard, I am assumming it has only one bit set. 
// Dropping these to square indexs, which should take up one byte apiece, we would save 14 bytes in the Move structure.
// However to do this, I have to anaylze all usages of .from and .to. To see the speed consequences.
// The tradeoffs are doing a bitboard_to_lowest_square() to get a square from a bitboard - .vs. the opposite - getting a 
// bitboard from a squarem which is simply a 1<< operation. Here is a suvery of usages of .to and .from, with my comments underneath.
// remember, my goal is store the square (1 byte) and not the bitboard like I do now (16 bytes). But I am concerned 
// about time consequences.
//
//    engine_communicator_make_move_two_acn()
//       Not sure whats going on here, but its not a hot line.
//
//    in_check_after_move_fast()
//       Here we would have to add two 1<< operations. The alogorith needs two bitmaps. This is a very hot path.
//
//    pushMove()
//       Two bitboard_to_lowest_square() calls exist to get the squares, these would go away if we store by square
//       Fairly hot path.
//
//    popMove()
//       Two bitboard_to_lowest_square() calls, to get the squares, these would go away if we store by square
//       Also a use of the 1-bit bb. We would add one 1<< here. Fairly hot path.
//
//    add_move_to_vector()
//        I store the things, inputs are 1-bit bbs. So we have to add two bitboard_to_lowest_square_fast() calls
//        This is a very hot path.
//
//    bitboards_to_algebraic()
//      Not sure whats going on here, but its not a hot line.
//
//    SEE_for_capture()
//      Muddled, but it seems like both  the bitboards and the squares are used. So we probably will
//     add two 1<< here, but remove two bitboard_to_lowest_square() go away. A Wash? This is a very hot path.
//
//    MoveSet2() etc. These are unchanged, and will only get faster. These are not used in hot lines I think.
//
//    move_to_string()
//       Not sure whats going on here, but its not a hot line.
//
//    The Move sturcture has an equality operater (==) using .to and .from, and here we win because now 
//    we compare less useless zeros.
//