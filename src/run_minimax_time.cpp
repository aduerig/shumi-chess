
#include <math.h>

#include <chrono>
#include <cstdio>
#include <iostream>
#include <limits>
#include <ostream>
#include <sstream>
#include <thread>

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

static const char* game_state_to_string(GameState state)
{
    switch (state) {
        case INPROGRESS: return "in progress";
        case WHITEWIN:   return "white wins";
        case BLACKWIN:   return "black wins";
        case DRAW:       return "draw";
        default:         return "unknown";
    }
}

static string move_to_uci(const Move& move)
{
    string move_text = utility::representation::move_to_string(move);
    char promo = utility::representation::piece_to_charactor(move.promotion);
    if (promo != ' ') {
        move_text += promo;
    }
    return move_text;
}

static void make_engine_move(Engine& engine, Move move)
{
    engine.users_last_move = move;
    engine.ply_so_far++;

    engine.gamePGN.addMe(move, engine);

    engine.move_history = stack<Move>();

    if (move.piece_type == Piece::NONE) {
        cout << "\x1b[1;31mNo move to make\x1b[0m" << endl;
        return;
    }

    // Make the move
    if (move.color == Color::WHITE) {
        engine.pushMove_t<Color::WHITE>(move);
    } else {
        engine.pushMove_t<Color::BLACK>(move);
    }

    // Manage tree time repetition
    engine.three_time_rep_stack.push_back(engine.game_board.zobrist_key);

    bool b_reversable = engine.game_board.isReversableMove(move);
    if (!b_reversable) {
        engine.boundary_stack.push_back((int)engine.three_time_rep_stack.size() - 1);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {

    assert(true);       // Keeps assert compilation visible without aborting this runner.

    // Make board
    //string FENString = "r2qnrk1/1p2ppbp/p5p1/2p1N3/b1B5/1PN5/1B1P1PPP/R1R1Q1K1 w - - 0 14";

    //string FENString = "r2q1rk1/1p2ppbp/N5p1/4N3/2n5/1P6/1B1P1PPP/R1R1Q1K1 b - - 0 16";


    // Make engine
    //Engine engine(FENString);
    Engine engine;

    //std::this_thread::sleep_for(std::chrono::seconds(3));   // debug only

    MinimaxAI minimax_ai(engine);

    // Show board
    string out = utility::representation::gameboard_to_string(engine.game_board);
    cout << out << endl;

    // Decide on arguments
    int time_to_use = 3000;
    int depth_to_use = 7;
    int max_ply_to_play = 10;
    if (argc < 2) {
        //cout << "You entered no argument for 'time_to_use', using default value of " << time_to_use << "msec" << endl;
    } else {
        time_to_use = atoi(argv[1]);
        //cout << "You entered time_to_use of: " << time_to_use << endl;
    }
    if (argc >= 3) {
        depth_to_use = atoi(argv[2]);
    }
    if (argc >= 4) {
        max_ply_to_play = atoi(argv[3]);
    }

    cout << "using level= " << depth_to_use
         << "  msec = " << time_to_use
         << "  max ply = " << max_ply_to_play << endl;

    GameState state = engine.is_game_over();
    for (int ply = 1; state == INPROGRESS && ply <= max_ply_to_play; ++ply) {
        Move move = minimax_ai.get_move_iterative_deepening(time_to_use, depth_to_use, 0);

        if (move.piece_type == Piece::NONE) {
            cout << "No legal move returned at ply " << ply << endl;
            break;
        }

        cout << "\nPly " << ply << " "
             << utility::representation::color_to_string(move.color)
             << " move: " << move_to_uci(move) << endl;

        make_engine_move(engine, move);

        out = utility::representation::gameboard_to_string(engine.game_board);
        cout << out << endl;

        state = engine.is_game_over();
    }

    cout << "Game state: " << game_state_to_string(state) << endl;
    cout << "PGN: " << engine.gamePGN.spitout() << endl;


    return 0;
}

//      run: cmake --build C:\programming\shumi-chess\build --config RelWithDebInfo
//      look in: C:\programming\shumi-chess\build\bin\RelWithDebInfo


