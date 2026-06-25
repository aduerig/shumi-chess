
#include <math.h>

#include <chrono>
#include <conio.h>
#include <cstdio>
#include <iostream>
#include <limits>
#include <ostream>
#include <sstream>
#include <thread>
#include <vector>


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

//////////////////////////////////////////////////////////////////////////////////////////////


#include <fstream>

static std::ofstream uci_debug_log(
    "C:\\programming\\shumi-chess\\uci_debug.txt",
    std::ios::app
);


///////////////////////////////////////////////////////////////////////////////////////////////

static void make_engine_move(Engine& engine, Move move)
{
    engine.users_last_move = move;
    engine.ply_so_far++;

    engine.gamePGN.addMe(move, engine);

    engine.move_history = stack<Move>();

    if (move.piece_type == Piece::NONE) {
        uci_debug_log << "\x1b[1;31mNo move to make\x1b[0m" << endl;
        return;
    }

    // Make the move
    if (move.color == Color::WHITE) {
        engine.pushMove_t<Color::WHITE>(move);
    } else {
        engine.pushMove_t<Color::BLACK>(move);
    }

    // Manage three time repetition
    engine.three_time_rep_stack.push_back(engine.game_board.zobrist_key);

    bool b_reversable = engine.game_board.isReversableMove(move);
    if (!b_reversable) {
        engine.boundary_stack.push_back((int)engine.three_time_rep_stack.size() - 1);
    }
}

static string move_to_uci(const Move& move)
{
    const ull movefrom = utility::bit::square_to_bitboard(move.fromSQ);
    const ull moveto = utility::bit::square_to_bitboard(move.toSQ);

    string move_uci = utility::representation::bitboard_to_acn_conversion(movefrom)
                    + utility::representation::bitboard_to_acn_conversion(moveto);

    if (move.promotion != Piece::NONE) {
        move_uci += utility::representation::piece_to_charactor(move.promotion);
    }

    return move_uci;
}

static bool make_uci_move(Engine& engine, const string& move_uci)
{
    vector<Move> legal_moves;
    engine.get_legal_moves_fast(engine.game_board.turn, false, false, legal_moves);

    for (const Move& move : legal_moves) {
        if (move_to_uci(move) == move_uci) {
            make_engine_move(engine, move);
            return true;
        }
    }

    uci_debug_log << "Invalid UCI move for current position: " << move_uci << endl;
    return false;
}

// Is old_moves a prefix list to new_moves?
static bool is_prefix(const vector<string>& old_moves, const vector<string>& new_moves)
{
    if (old_moves.size() > new_moves.size()) {
        return false;
    }

    for (size_t i = 0; i < old_moves.size(); i++) {
        if (old_moves[i] != new_moves[i]) {
            return false;
        }
    }

    return true;
}

static bool parse_position_command(const string& line, string& new_base, vector<string>& new_moves)
{
    istringstream input(line);
    vector<string> tokens;
    string token;

    while (input >> token) {
        tokens.push_back(token);
    }

    if (tokens.size() < 2 || tokens[0] != "position") {
        return false;
    }

    size_t next_token = 0;

    if (tokens[1] == "startpos") {
        new_base = "startpos";
        next_token = 2;
    } else if (tokens[1] == "fen") {
        if (tokens.size() < 8) {
            return false;
        }

        new_base = tokens[2];
        for (size_t i = 3; i < 8; i++) {
            new_base += " " + tokens[i];
        }
        next_token = 8;
    } else {
        return false;
    }

    new_moves.clear();

    if (next_token == tokens.size()) {
        return true;
    }

    if (tokens[next_token] != "moves") {
        return false;
    }

    new_moves.assign(tokens.begin() + next_token + 1, tokens.end());
    return true;
}

static bool create_position(const string& base,
                            const vector<string>& moves,
                            Engine*& engine,
                            MinimaxAI*& minimax_ai)
{
    static const string STARTPOS_FEN =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    const string& fen = base == "startpos" ? STARTPOS_FEN : base;
    Engine* new_engine = new Engine(fen);
    MinimaxAI* new_minimax_ai = new MinimaxAI(*new_engine);

    for (const string& move_uci : moves) {
        if (!make_uci_move(*new_engine, move_uci)) {
            delete new_minimax_ai;
            delete new_engine;
            return false;
        }
    }

    delete minimax_ai;
    delete engine;
    engine = new_engine;
    minimax_ai = new_minimax_ai;
    return true;
}



///////////////////////////////////////////////////////////////////////////////////////////////////



int main()
{
    int iMovesInGame = 0;

    // 
    // Decide on Shumi engine chess arguments
    //     8, 400 is about 40 moves in 15 min.
    //
    int depth_to_use = 8;
    int time_to_use = 800;
    //int max_ply_to_play = 4;
    int player_id = UNCLE_SHUMI;       //  UNCLE_SHUMI;
    int flags = _FEATURE_ENHANCED_DEPTH_TT2 | _FEATURE_TT2 | _FEATURE_KILLER | _FEATURE_UNQUIET_SORT;


    int iRandomMoves = 0;
    if (iMovesInGame < 2) iRandomMoves = 1;




    constexpr int MAX_FENS = 10;
    string FENs[MAX_FENS];


    FENs[0] = "rnbqk2r/ppp2ppp/3b4/3p4/3Pn3/2PB1N2/PP3PPP/RNBQK2R w KQkq - 1 8";        // Random Petrov
    FENs[1] = "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2BPP3/2P2N2/PP3PPP/RNBQK2R b KQkq d3 0 5";  // Giaco
    int iPositions = 0;

    Engine* engine = nullptr;
    MinimaxAI* minimax_ai = nullptr;
    bool have_position = false;
    std::string current_base;
    std::vector<std::string> moves_so_far;

    std::string line;



    while (std::getline(std::cin, line)) {

        uci_debug_log << line << endl;

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
            current_base.clear();
            moves_so_far.clear();
            have_position = false;

        } else if (line.rfind("position ", 0) == 0) {
            // set board from "startpos" or "fen"
            // then play the listed moves

            //  codex resume 019ef7c7-19ae-7220-a0b9-e6318c748810
            // position startpos
            // position startpos moves e2e4 e7e5 g1f3
            // position fen <FEN>
            // position fen <FEN> moves e2e4 e7e5

            string new_base;
            vector<string> new_moves;

            if (!parse_position_command(line, new_base, new_moves)) {
                uci_debug_log << "Invalid position command: " << line << endl;
                continue;
            }

            bool position_updated = false;

            if (!have_position || engine == nullptr || minimax_ai == nullptr) {

                iMovesInGame = 0;
                position_updated = create_position(new_base, new_moves, engine, minimax_ai);

            } else if (new_base == current_base && is_prefix(moves_so_far, new_moves)) {

                position_updated = true;

                for (size_t i = moves_so_far.size(); i < new_moves.size(); i++) {
                    if (!make_uci_move(*engine, new_moves[i])) {
                        position_updated = false;
                        break;
                    }
                }

                if (!position_updated) {
                    iMovesInGame = 0;
                    position_updated = create_position(new_base, new_moves, engine, minimax_ai);
                }
            } else {
                iMovesInGame = 0;
                position_updated = create_position(new_base, new_moves, engine, minimax_ai);
            }

            if (!position_updated) {
                uci_debug_log << "Could not apply position command: " << line << endl;
                continue;
            }

            current_base = new_base;
            moves_so_far = new_moves;
            have_position = true;

            //Engine engine(FENs[iPositions]);
            //Engine engine;

            //std::this_thread::sleep_for(std::chrono::seconds(3));   // debug only

            //MinimaxAI minimax_ai(*engine);

            // Show board
            // string out = utility::representation::gameboard_to_string(engine->game_board);
            // uci_debug_log << out << endl;



        } else if (line == "go" || line.rfind("go ", 0) == 0) {

            if (!have_position || engine == nullptr || minimax_ai == nullptr) {
                vector<string> no_moves;

                if (!create_position("startpos", no_moves, engine, minimax_ai)) {
                    std::cout << "bestmove 0000\n";
                    std::cout.flush();
                    continue;
                }

                current_base = "startpos";
                moves_so_far.clear();
                have_position = true;
            }

         

            //
            // Get "best move" from Shumi
  

            Move move = minimax_ai->get_move_iterative_deepening(time_to_use, depth_to_use, player_id, iRandomMoves, flags);

            if (move.piece_type == Piece::NONE) {
                cerr << "No legal move returned at ply " << endl;
                std::cout << "bestmove 0000\n";
                std::cout.flush();
                continue;
            }

            // Translate this Move into UCI coordinate notation
            string move_str = move_to_uci(move);

            //
            // Make Shumi move in the Shumi engine
            make_engine_move(*engine, move);
            moves_so_far.push_back(move_str);


            int nodesSeen = minimax_ai->nodes_visited;

            //
            // Show move info
            // options to the "info" command sent to the "GUI"
            //
            // depth 8                  // search depth reached
            // seldepth 14              // deepest selective/qsearch depth reached
            // time 1234                // elapsed search time in milliseconds
            // nodes 456789             // total nodes searched
            // nps 1234567              // nodes per second
            // score cp 34              // score in centipawns
            // score mate 3             // mate in 3
            // score cp 34 lowerbound   // score is at least this good
            // score cp 34 upperbound   // score is at most this good
            // pv e2e4 e7e5 g1f3        // principal variation
            // multipv 2                // this is the second-best PV line
            // currmove e2e4            // move currently being searched
            // currmovenumber 5         // current move number in the move list
            // hashfull 123             // hash fullness, 0 to 1000
            // tbhits 0                 // tablebase hits
            // cpuload 850              // CPU load, 0 to 1000
            // string text here         // debug/status text
            // refutation e2e4 e7e5     // refutation line for a move
            // currline 1 e2e4 e7e5     // current line for CPU/thread 1

            //int nps = 1234567;
            int nps = minimax_ai->iNodes_per_Second;
            int centiPawnsRel = (int)(minimax_ai->d_best_move_score_rel * 100.0);
            //std::cout << "info nps " << nps << "\n";
            std::cout << "info" 
                    << " depth " << minimax_ai->max_attained_depth
                    << " score cp " << centiPawnsRel
                    << " nodes " << nodesSeen
                    << " nps " << nps
                    << "\n";


            // Show move
            iMovesInGame++;
            std::cout << "bestmove " << move_str << "\n";
            std::cout.flush();

            // // cerr << "\nPly " << ply << " "
            // //      << utility::representation::color_to_string(move.color)
            // //      << " move: " << move_to_uci(move) << endl;
            // std::cout << "bestmove " << from_str << to_str << cpromo << "\n";
            std::cout.flush();

        } else if (line == "quit") {
            uci_debug_log << "quit received\n";
            uci_debug_log.flush();
            uci_debug_log.close();
            break;
        }
    }

    delete minimax_ai;
    delete engine;

    return 0;
}
