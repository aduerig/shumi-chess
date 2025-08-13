#include <float.h>
#include <bitset>
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


int MinimaxAI::bits_in(ull bitboard) {
    auto bs = bitset<64>(bitboard);
    return (int) bs.count();
}

template<class T>
string format_with_commas(T value) {
    stringstream ss;
    ss.imbue(locale(""));
    ss << fixed << value;
    return ss.str();
}

double MinimaxAI::evaluate_board(Color for_color, vector<ShumiChess::Move>& moves) {
    double board_val_adjusted = 0;

    for (const auto& color : array<Color, 2>{Color::WHITE, Color::BLACK}) {
        double d_board_val = 0;
        // Add up the values for each piece
        for (const auto& i : piece_values) {
            
            // Obtain piece type and value
            Piece piece_type;
            double piece_value;
            tie(piece_type, piece_value) = i;
            
            // Since the king can never leave the board (engine does not allow), and there is an infinite score checked earlier we can just skip kings
            if (piece_type == Piece::KING) continue;

            ull pieces_bitboard = engine.game_board.get_pieces(color, piece_type);

            d_board_val += (piece_value * bits_in(pieces_bitboard));
            
            // NOTE: This is adding a small bonus for pawns and knights in the middle of the board
            if (piece_type == Piece::PAWN || piece_type == Piece::KNIGHT) {
                ull middle_place = pieces_bitboard & (0b00000000'00000000'00000000'00011000'00011000'00000000'00000000'00000000);
                d_board_val +=  0.1 * bits_in(middle_place);
                // cout << "adding up " << color_str(color) << endl;
            }
        }
        // Negative score for opposite pieces
        if (color != for_color) {
            d_board_val *= -1;
        }
        board_val_adjusted += d_board_val;
    }
    return board_val_adjusted + (moves.size() / 80);
}


tuple<double, Move> MinimaxAI::store_board_values_negamax(int depth, double alpha, double beta, unordered_map<uint64_t, unordered_map<Move, double, MoveHash>> &board_values, bool debug) {
    nodes_visited++;
    vector<Move> moves = engine.get_legal_moves();
    GameState state = engine.game_over(moves);
    
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
        double eval = evaluate_board(engine.game_board.turn, moves);
        if (debug == true) {
            cout << colorize(AColor::BRIGHT_BLUE, "===== DEPTH 0 EVAL: " + to_string(eval) + ", color is: " + color_str(engine.game_board.turn)) << endl;
            print_gameboard(engine.game_board);
        }
        return make_tuple(eval, Move{});
    }

    unordered_map<Move, double, MoveHash> moves_with_values;
    std::vector<Move> sorted_moves;

    if (board_values.find(engine.game_board.zobrist_key) != board_values.end()) {
        moves_with_values = board_values[engine.game_board.zobrist_key];

        for (auto& something : moves_with_values) {
            Move looking = something.first;
            if (std::find(moves.begin(), moves.end(), looking) == moves.end()) {
                print_gameboard(engine.game_board);
                cout << "Move shouldnt be legal: " << move_to_string(looking) << " at depth 1" << endl;
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
        for (Move& m : moves) {
            if (moves_with_values.find(m) == moves_with_values.end()) {
                sorted_moves.push_back(m);
            }
        }
    } else {
        sorted_moves = moves;
    }

    double dMax_move_value = -DBL_MAX;
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

        if (score_value > dMax_move_value) {
            dMax_move_value = score_value;
            best_move = m;
        }
        alpha = max(alpha, dMax_move_value);
        if (alpha >= beta) {
            break;
        }
    }
    return make_tuple(dMax_move_value, best_move);
}




Move MinimaxAI::get_move_iterative_deepening(double time) {
    seen_zobrist.clear();
    nodes_visited = 0;

    uint64_t zobrist_key_start = engine.game_board.zobrist_key;
    cout << "zobrist_key at start of get_move_iterative_deepening is: " << zobrist_key_start << endl;

    auto start_time = chrono::high_resolution_clock::now();
    auto required_end_time = start_time + chrono::duration<double>(time);

    unordered_map<uint64_t, unordered_map<Move, double, MoveHash>> board_values;
    Move best_move;
    double best_move_value;

    int depth = 1;
    while (chrono::high_resolution_clock::now() <= required_end_time) {
        cout << "Deepening to " << depth << endl;
        auto ret_val = store_board_values_negamax(depth, -DBL_MAX, DBL_MAX, board_values, false);
        best_move_value = get<0>(ret_val);
        best_move = get<1>(ret_val);
        depth++;
    }

    vector<Move> top_level_moves = engine.get_legal_moves();
    Move move_chosen = top_level_moves[0];

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
    cout << "get_move_iterative_deepening zobrist_key at begining: " << zobrist_key_start << ", at end: " << engine.game_board.zobrist_key << endl;
    return best_move;
}









// === straight forward minimax below ===
double MinimaxAI::get_value(int depth, int color_multiplier, double alpha, double beta) {
    nodes_visited++;
    vector<Move> moves = engine.get_legal_moves();
    GameState state = engine.game_over(moves);
    
    if (state == GameState::BLACKWIN) {
        return (-DBL_MAX + 1) * (color_multiplier);
    }
    else if (state == GameState::WHITEWIN) {
        return (DBL_MAX - 1) * (color_multiplier);
    }
    else if (state == GameState::DRAW) {
        return 0;
    }
    
    // Depth decreases, when it hits zero we stop looking.
    if (depth == 0) {
        Color color_perspective = Color::BLACK;
        if (color_multiplier) {
            color_perspective = Color::WHITE;
        }
        return evaluate_board(color_perspective, moves) * color_multiplier;
    }
    //
    // Otherwise dive down one more level.
    //
    if (color_multiplier == 1) {  // Maximizing player
        double dMax_move_value = -DBL_MAX;
        for (Move& m : moves) {
            engine.push(m);
            double score_value = -1 * get_value(depth - 1, color_multiplier * -1, alpha, beta);
            engine.pop();
            dMax_move_value = max(dMax_move_value, score_value);
            alpha = max(alpha, dMax_move_value);
            if (beta <= alpha) {
                break; // beta cut-off
            }
        }
        return dMax_move_value;

    } else {  // Minimizing player
        double dMin_move_value = DBL_MAX;
        for (Move& m : moves) {
            engine.push(m);
            double score_value = -1 * get_value(depth - 1, color_multiplier * -1, alpha, beta);
            engine.pop();
            dMin_move_value = min(dMin_move_value, score_value);
            beta = min(beta, dMin_move_value);
            if (beta <= alpha) {
                break; // alpha cut-off
            }
        }
        return dMin_move_value;
    }
}

//
// Get 
//

Move MinimaxAI::get_move(int depth) {
    auto start_time = chrono::high_resolution_clock::now();

    nodes_visited = 0;

    int color_multiplier = 1;
    if (engine.game_board.turn == Color::BLACK) {
        color_multiplier = -1;
    }

    Move move_chosen;
    double dMax_move_value = -DBL_MAX;
    vector<Move> moves = engine.get_legal_moves();
    for (Move& m : moves) {
        engine.push(m);
        double score_value = get_value(depth - 1, color_multiplier * -1, -DBL_MAX, DBL_MAX);
        if (score_value * -1 > dMax_move_value) {
            dMax_move_value = score_value * -1;
            move_chosen = m;
        }
        engine.pop();
    }
    
    string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";
    cout << colorize(AColor::BRIGHT_CYAN, "Minimax AI get_move chose move: for " + color + " player") << endl;
    cout << colorize(AColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + " nodes total") << endl;

    chrono::duration<double> total_time = chrono::high_resolution_clock::now() - start_time;
    cout << colorize(AColor::BRIGHT_GREEN, "Total time taken for get_move(): " + to_string(total_time.count()) + " s") << endl;

    return move_chosen;
}

Move MinimaxAI::get_move() {
    int default_depth = 7;
    cout << colorize(AColor::BRIGHT_GREEN, "Called MinimaxAI::get_move() with no arguments, using default depth: " + to_string(default_depth)) << endl;
    return get_move(default_depth);
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