#include <float.h>
#include <iomanip>
#include <sstream>
#include <locale>

#include "minimax.hpp"
#include "utility.hpp"

using namespace std;
using namespace ShumiChess;
using namespace utility;
using namespace utility::representation;
using namespace utility::bit;

MinimaxAI::MinimaxAI(Engine& e) : engine(e) { }


template<class T>
string format_with_commas(T value) {
    stringstream ss;
    ss.imbue(locale(""));
    ss << fixed << value;
    return ss.str();
}

double MinimaxAI::evaluate_board(Color for_color, tuple<Move*, int> all_moves_tuple) {
    double board_val_adjusted = 0;
    for (const auto& color : color_arr) {
        double board_val = 0;
        for (int i = 0; i < 6; i++) {
            Piece piece_type = all_pieces[i];
            double piece_value = all_piece_values[i];
            
            if (piece_type == Piece::KING) {
                continue;
            }
            ull pieces_bitboard = engine.game_board.get_pieces(color, piece_type);

            board_val += piece_value * bits_in(pieces_bitboard);
            
            if (piece_type == Piece::PAWN || piece_type == Piece::KNIGHT) {
                ull middle_place = pieces_bitboard & (0b00000000'00000000'00000000'00011000'00011000'00000000'00000000'00000000);
                board_val +=  0.1 * bits_in(middle_place);
                // cout << "adding up " << color_str(color) << endl;
            }
        }
        if (color != for_color) {
            board_val *= -1;
        }
        board_val_adjusted += board_val;
    }
    return board_val_adjusted + get<1>(all_moves_tuple) / 80;
}


tuple<double, Move> MinimaxAI::store_board_values_negamax(int depth, double alpha, double beta, spp::sparse_hash_map<uint64_t, spp::sparse_hash_map<Move, double, MoveHash>> &board_values, bool debug) {
    nodes_visited++;
    tuple<Move*, int> all_moves_tuple = engine.get_legal_moves();
    Move* all_moves = get<0>(all_moves_tuple);
    int total_moves = get<1>(all_moves_tuple);
    GameState state = engine.game_over(all_moves_tuple);
    
    if (state != GameState::INPROGRESS) {
        double end_value = 0;
        if (state == GameState::BLACKWIN) {
            end_value = engine.game_board.turn == ShumiChess::WHITE ? -DBL_MAX + 1 : DBL_MAX - 1;
        }
        else if (state == GameState::WHITEWIN) {
            end_value = engine.game_board.turn == ShumiChess::BLACK ? -DBL_MAX + 1 : DBL_MAX - 1;
        }
        return make_tuple(end_value, Move{});
    }
    
    if (depth == 0) {
        double eval = evaluate_board(engine.game_board.turn, all_moves_tuple);
        if (debug == true) {
            cout << colorize(AColor::BRIGHT_BLUE, "===== DEPTH 0 EVAL: " + to_string(eval) + ", color is: " + color_str(engine.game_board.turn)) << endl;
            print_gameboard(engine.game_board);
        }
        return make_tuple(eval, Move{});
    }

    spp::sparse_hash_map<Move, double, MoveHash> moves_with_values;
    std::vector<Move> sorted_moves;
    sorted_moves.reserve(total_moves);

    if (board_values.find(engine.game_board.zobrist_key) != board_values.end()) {
        moves_with_values = board_values[engine.game_board.zobrist_key];

        for (auto& something : moves_with_values) {
            bool found = false;
            for (int i = 0; i < total_moves; i++) {
                if (all_moves[i] == something.first) {
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

        std::vector<std::pair<Move, double>> vec(moves_with_values.begin(), moves_with_values.end());

        std::sort(vec.begin(), vec.end(), 
            [](const std::pair<Move, double>& a, const std::pair<Move, double>& b) {
                return a.second > b.second;
            });
        
        for (const auto& pair : vec) {
            sorted_moves.push_back(pair.first);
        }
        for (int i = 0; i < total_moves; i++) {
            Move m = all_moves[i];
            if (moves_with_values.find(m) == moves_with_values.end()) {
                sorted_moves.push_back(m);
            }
        }
    } else {
        for (int i = 0; i < total_moves; i++) {
            sorted_moves.push_back(all_moves[i]);
        }
    }

    double max_move_value = -DBL_MAX;
    Move best_move = sorted_moves[0];
    for (Move &m : sorted_moves) {
        engine.push(m);
        auto ret_val = store_board_values_negamax(depth - 1, -beta, -alpha, board_values, debug);
        double score_value = -get<0>(ret_val);
        if (debug == true) {
            cout << colorize(AColor::BRIGHT_GREEN, "On depth " + to_string(depth) + ", move is: " + move_to_string(m) + ", score_value below is: " + to_string(score_value) + ", color perspective: " + color_str(engine.game_board.turn)) << endl;
            print_gameboard(engine.game_board);
        }

        engine.pop();
        board_values[engine.game_board.zobrist_key][m] = score_value;

        if (score_value > max_move_value) {
            max_move_value = score_value;
            best_move = m;
        }
        alpha = max(alpha, max_move_value);
        if (alpha >= beta) {
            break;
        }
    }
    return make_tuple(max_move_value, best_move);
}




Move MinimaxAI::get_move_iterative_deepening(double time) {
    seen_zobrist.clear();
    nodes_visited = 0;

    uint64_t zobrist_key_start = engine.game_board.zobrist_key;

    auto start_time = chrono::high_resolution_clock::now();
    auto required_end_time = start_time + chrono::duration<double>(time);

    spp::sparse_hash_map<uint64_t, spp::sparse_hash_map<Move, double, MoveHash>> board_values;
    Move best_move;
    double best_move_value;

    int depth = 1;
    while (chrono::high_resolution_clock::now() <= required_end_time) {
        if (depth < 40) {
            cout << "Deepening to " << depth << endl;
        }
        auto ret_val = store_board_values_negamax(depth, -DBL_MAX, DBL_MAX, board_values, false);
        best_move_value = get<0>(ret_val);
        best_move = get<1>(ret_val);
        depth++;
    }
    // cout << "finsished store_board_values_negamax" << endl;
    // exit(1);

    tuple<Move*, int> moves_tuple = engine.get_legal_moves();
    Move* all_moves = get<0>(moves_tuple);
    int num_moves = get<1>(moves_tuple);
    Move move_chosen = all_moves[0];

    cout << "Went to depth " << depth - 1 << endl;
    cout << "Found " << board_values.size() << " items inside of board_values" << endl;

    if (board_values.find(engine.game_board.zobrist_key) == board_values.end()) {
        cout << "Did not find zobrist key in board_values, this is bad" << endl;
        exit(1);
    }

    // auto stored_move_values = board_values[engine.game_board.zobrist_key];
    // for (Move& m : top_level_moves) {
    //     if (stored_move_values.find(m) == stored_move_values.end()) {
    //         cout << "did not find move " << move_to_string(m) << " in the stored moves. this is probably bad" << endl;
    //         exit(1);
    //     }
    // }
    string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";
    cout << colorize(AColor::BRIGHT_CYAN, "Minimax AI get_move_iterative_deepening chose move: for " + color + " player with score of " + to_string(best_move_value)) << endl;
    cout << colorize(AColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + " nodes total") << endl;
    cout << colorize(AColor::BRIGHT_BLUE, "Time it was supposed to take: " + to_string(time) + " s") << endl;
    cout << colorize(AColor::BRIGHT_GREEN, "Actual time taken: " + to_string(chrono::duration<double>(chrono::high_resolution_clock::now() - start_time).count()) + " s") << endl;
    if (engine.game_board.zobrist_key != zobrist_key_start) {
        cout << "get_move_iterative_deepening zobrist_key at begining: " << zobrist_key_start << ", at end: " << engine.game_board.zobrist_key << endl;
        exit(1);
    }
    return best_move;
}





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