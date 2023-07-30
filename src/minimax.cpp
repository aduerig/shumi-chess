#include <float.h>
#include <iomanip>
#include <sstream>
#include <locale>

#include "globals.hpp"
#include "minimax.hpp"
#include "utility.hpp"

using namespace std;
using namespace ShumiChess;
using namespace utility;
using namespace utility::representation;
using namespace utility::bit;

MinimaxAI::MinimaxAI(Engine& e) : engine(e) {
    // std::array<std::array<double, 8>, 8> pawn_values = {{
    //     {5, 5, 7, 7, 0, 0, 0, 0},
    //     {4, 6, 6, 8, 8, 8, 8, 0},
    //     {5, 5, 7, 7, 7, 7, 8, 0},
    //     {4, 6, 6, 6, 6, 6, 8, 0},
    //     {5, 5, 5, 5, 5, 5, 8, 7},
    //     {4, 4, 4, 4, 4, 7, 6, 7},
    //     {3, 3, 3, 3, 6, 5, 6, 5},
    //     {2, 2, 2, 5, 4, 5, 4, 5},
    // }};

    // std::array<double, 64> black_pawn_value_lookup;
    // std::array<double, 64> white_pawn_value_lookup;

    for (int i = 0; i < 64; i++) {
        int row = i / 8;
        int col = i % 8;
        black_pawn_value_lookup[i] = .1 * pawn_values[7 - row][7 - col];
        white_pawn_value_lookup[63 - i] = .1 * pawn_values[7 - row][7 - col];
    }
}


template<class T>
string format_with_commas(T value) {
    stringstream ss;
    ss.imbue(locale(""));
    ss << fixed << value;
    return ss.str();
}

MoveAndBoardValue MinimaxAI::store_board_values_negamax(int depth, double alpha, double beta, spp::sparse_hash_map<uint64_t, spp::sparse_hash_map<Move, double, MoveHash>> &board_values, spp::sparse_hash_map<int, MoveBoardValueDepth> &transposition_table, bool debug) {
    nodes_visited++;
    LegalMoves legal_moves = engine.get_legal_moves();
    GameState state = engine.game_over(legal_moves);
    
    if (state != GameState::INPROGRESS) {
        double end_value = 0;
        if (state == GameState::BLACKWIN) {
            end_value = engine.game_board.turn == ShumiChess::WHITE ? -DBL_MAX + 1 : DBL_MAX - 1;
        }
        else if (state == GameState::WHITEWIN) {
            end_value = engine.game_board.turn == ShumiChess::BLACK ? -DBL_MAX + 1 : DBL_MAX - 1;
        }
        return {Move{}, end_value};
    }
    
    if (depth == 0) {
        double eval = evaluate_board<Piece::PAWN>(engine.game_board.turn, legal_moves) + evaluate_board<Piece::ROOK>(engine.game_board.turn, legal_moves) + evaluate_board<Piece::KNIGHT>(engine.game_board.turn, legal_moves) + evaluate_board<Piece::QUEEN>(engine.game_board.turn, legal_moves) + evaluate_board<Piece::NONE>(engine.game_board.turn, legal_moves);
        if (debug == true) {
            cout << colorize(AColor::BRIGHT_BLUE, "===== DEPTH 0 EVAL: " + to_string(eval) + ", color is: " + color_str(engine.game_board.turn)) << endl;
            print_gameboard(engine.game_board);
        }
        return {Move{}, eval};
    }

    if (transposition_table.find(engine.game_board.zobrist_key) != transposition_table.end()) {
        MoveBoardValueDepth mbvd = transposition_table[engine.game_board.zobrist_key];
        if (mbvd.depth >= depth) {
            return {mbvd.move, mbvd.board_value};
        }
    }

    spp::sparse_hash_map<Move, double, MoveHash> moves_with_values;
    std::vector<Move> sorted_moves;
    sorted_moves.reserve(legal_moves.num_moves);

    if (board_values.find(engine.game_board.zobrist_key) != board_values.end()) {
        moves_with_values = board_values[engine.game_board.zobrist_key];

        for (auto& something : moves_with_values) {
            bool found = false;
            for (int i = 0; i < legal_moves.num_moves; i++) {
                if (legal_moves.moves[i] == something.first) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                print_gameboard(engine.game_board);
                cout << "Move shouldnt be legal: " << move_to_string(something.first) << " at depth 1" << endl;
                exit(1);
            }
        }

        std::vector<MoveAndBoardValue> vec;
        vec.reserve(moves_with_values.size());
        for (const auto& pair : moves_with_values) {
            vec.push_back({pair.first, pair.second});
        }

        std::sort(vec.begin(), vec.end(), 
            [](const MoveAndBoardValue& a, const MoveAndBoardValue& b) {
                return a.board_value > b.board_value;
            });
        
        for (const auto& move_and_board_value : vec) {
            sorted_moves.push_back(move_and_board_value.move);
        }
        for (int i = 0; i < legal_moves.num_moves; i++) {
            Move m = legal_moves.moves[i];
            if (moves_with_values.find(m) == moves_with_values.end()) {
                sorted_moves.push_back(m);
            }
        }
    } else {
        for (int i = 0; i < legal_moves.num_moves; i++) {
            sorted_moves.push_back(legal_moves.moves[i]);
        }
    }

    double max_move_value = -DBL_MAX;
    Move best_move = sorted_moves[0];
    for (Move &m : sorted_moves) {
        engine.push(m);
        MoveAndBoardValue move_and_board_value = store_board_values_negamax(depth - 1, -beta, -alpha, board_values, transposition_table, debug);
        double board_value = -move_and_board_value.board_value;

        engine.pop();
        board_values[engine.game_board.zobrist_key][m] = board_value;

        if (board_value > max_move_value) {
            max_move_value = board_value;
            best_move = m;
        }
        alpha = max(alpha, max_move_value);
        if (alpha >= beta) {
            break;
        }
    }
    transposition_table[engine.game_board.zobrist_key] = {best_move, max_move_value, depth};
    return {best_move, max_move_value};
}


Move MinimaxAI::get_move_iterative_deepening(double time) {
    seen_zobrist.clear();
    nodes_visited = 0;

    uint64_t zobrist_key_start = engine.game_board.zobrist_key;

    auto start_time = chrono::high_resolution_clock::now();
    auto required_end_time = start_time + chrono::duration<double>(time);

    spp::sparse_hash_map<uint64_t, spp::sparse_hash_map<Move, double, MoveHash>> board_values;
    spp::sparse_hash_map<int, MoveBoardValueDepth> transposition_table;
    MoveAndBoardValue best_move;

    int depth = 1;
    while (chrono::high_resolution_clock::now() <= required_end_time) {
        best_move = store_board_values_negamax(depth, -DBL_MAX, DBL_MAX, board_values, transposition_table, false);
        if (best_move.board_value == (DBL_MAX - 1) || best_move.board_value == (-DBL_MAX + 1)) {
            break;
        }
        depth++;
    }

    LegalMoves legal_moves = engine.get_legal_moves();
    Move move_chosen = legal_moves.moves[0];

    cout << "Went to depth " << depth - 1 << endl;
    cout << "Found " << format_with_commas(board_values.size()) << " items inside of board_values" << endl;

    if (board_values.find(engine.game_board.zobrist_key) == board_values.end()) {
        cout << "Did not find zobrist key in board_values, this is bad" << endl;
        exit(1);
    }

    auto stored_move_values = board_values[engine.game_board.zobrist_key];
    for (int i = 0; i < legal_moves.num_moves; i++) {
        Move m = legal_moves.moves[i];
        if (stored_move_values.find(m) == stored_move_values.end()) {
            cout << "did not find move " << move_to_string(m) << " in the stored moves. this is probably bad" << endl;
            exit(1);
        }
    }
    string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";
    cout << colorize(AColor::BRIGHT_CYAN, "Minimax AI get_move_iterative_deepening chose move: for " + color + " player with score of " + to_string(best_move.board_value)) << endl;
    cout << colorize(AColor::BRIGHT_MAGENTA, "Nodes per second: " + format_with_commas((int)(nodes_visited / chrono::duration<double>(chrono::high_resolution_clock::now() - start_time).count()))) << endl;
    cout << colorize(AColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + " nodes total") << endl;
    cout << colorize(AColor::BRIGHT_BLUE, "Time it was supposed to take: " + to_string(time) + "s, Actual time taken: " + to_string(chrono::duration<double>(chrono::high_resolution_clock::now() - start_time).count()) + "s") << endl;
    if (engine.game_board.zobrist_key != zobrist_key_start) {
        cout << "get_move_iterative_deepening zobrist_key at begining: " << zobrist_key_start << ", at end: " << engine.game_board.zobrist_key << endl;
        exit(1);
    }
    return best_move.move;
}



// if (debug == true) {
//     cout << colorize(AColor::BRIGHT_GREEN, "On depth " + to_string(depth) + ", move is: " + move_to_string(m) + ", board_value below is: " + to_string(board_value) + ", color perspective: " + color_str(engine.game_board.turn)) << endl;
//     print_gameboard(engine.game_board);
// }




// uint64_t starting_zobrist_key = engine.game_board.zobrist_key;

// if (seen_zobrist.find(starting_zobrist_key) != seen_zobrist.end()) {
//     if (seen_zobrist[starting_zobrist_key] != gameboard_to_string(engine.game_board)) {
//         cout << "Zobrist collision!" << endl;
//         cout << "Zobrist key: " << starting_zobrist_key << endl;
//         cout << "Seen zobrist:\n" << seen_zobrist[starting_zobrist_key] << endl;
//         cout << "Current zobrist:\n" << gameboard_to_string(engine.game_board) << endl;
//         exit(1);
//     }
// }
// else {
//     seen_zobrist[starting_zobrist_key] = gameboard_to_string(engine.game_board);
// }


// if (engine.game_board.zobrist_key != starting_zobrist_key) {
//     print_gameboard(engine.game_board);
//     cout << "zobrist key doesn't match for: " << move_to_string(m) << endl;
//     exit(1);
// }