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


int MinimaxAI::evaluate_board() {
    return evaluate_board(engine.game_board.turn);
}

int MinimaxAI::bits_in(ull bitboard)
{
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

int MinimaxAI::evaluate_board(Color color) {
    Color opposite_turn = get_opposite_color(color);

    piece_and_values = {
        {engine.game_board.get_pieces_template<Piece::PAWN>(color), 1},
        {engine.game_board.get_pieces_template<Piece::ROOK>(color), 5},
        {engine.game_board.get_pieces_template<Piece::KNIGHT>(color), 3},
        {engine.game_board.get_pieces_template<Piece::BISHOP>(color), 3},
        {engine.game_board.get_pieces_template<Piece::QUEEN>(color), 8},

        {engine.game_board.get_pieces_template<Piece::PAWN>(opposite_turn), -1},
        {engine.game_board.get_pieces_template<Piece::ROOK>(opposite_turn), -5},
        {engine.game_board.get_pieces_template<Piece::KNIGHT>(opposite_turn), -3},
        {engine.game_board.get_pieces_template<Piece::BISHOP>(opposite_turn), -3},
        {engine.game_board.get_pieces_template<Piece::QUEEN>(opposite_turn), -8},
    };

    int total_board_val = 0;
    for (const auto& i : piece_and_values) {
        total_board_val += i.second * bits_in(i.first);
        // while (piece_bitboard) {
        //     bit::lsb_and_pop(piece_bitboard);
        //     total_board_val += piece_value;
        // }
    }
    return total_board_val;
}


double MinimaxAI::store_board_values(int depth, Color color, double alpha, double beta, unordered_map<int, unordered_map<Move, double, MoveHash>> &board_values) {
    int starting_zobrist_key = engine.game_board.zobrist_key;

    nodes_visited++;
    vector<Move> moves = engine.get_legal_moves();
    GameState state = engine.game_over(moves);
    
    if (state == GameState::BLACKWIN) {
        return -DBL_MAX + 1;
    }
    else if (state == GameState::WHITEWIN) {
        return DBL_MAX - 1;

    }
    else if (state == GameState::DRAW) {
        return 0;
    }
    
    if (depth == 0) {
        return evaluate_board(WHITE);
    }

    unordered_map<Move, double, MoveHash> moves_with_values;
    std::vector<Move> sorted_moves;

    if (board_values.find(engine.game_board.zobrist_key) != board_values.end()) {
        moves_with_values = board_values[engine.game_board.zobrist_key];

        for (auto& something : moves_with_values) {
            Move looking = something.first;
            if (std::find(moves.begin(), moves.end(), looking) == moves.end()) {
                cout << "Move shouldnt be legal: " << move_to_string(looking) << " at depth 1" << endl;
                exit(1);
            }
        }


        std::vector<std::pair<Move, double>> vec(moves_with_values.begin(), moves_with_values.end());

        if (color == Color::WHITE) {
            std::sort(vec.begin(), vec.end(), 
                [](const std::pair<Move, double>& a, const std::pair<Move, double>& b) {
                    return a.second > b.second;
                });
        }
        else {
            std::sort(vec.begin(), vec.end(), 
                [](const std::pair<Move, double>& a, const std::pair<Move, double>& b) {
                    return a.second < b.second;
                });
        }

        for (const auto& pair : vec) {
            sorted_moves.push_back(pair.first);
        }
        for (Move& m : moves) {
            if (moves_with_values.find(m) == moves_with_values.end()) {
                // cout << "this is weird... depth: " << depth << endl;
                sorted_moves.push_back(m);
            }
        }
    } else {
        sorted_moves = moves;
    }

    if (color == Color::WHITE) {  // Maximizing player
        double max_move_value = -DBL_MAX;

        for (Move &m : sorted_moves) {
            engine.push(m);
            double score_value = store_board_values(depth - 1, Color::BLACK, alpha, beta, board_values);
            engine.pop();
            board_values[engine.game_board.zobrist_key][m] = score_value;

            max_move_value = max(max_move_value, score_value);
            alpha = max(alpha, max_move_value);
            if (beta <= alpha) {
                break;
            }
        }

        return max_move_value;
    } else {
        double min_move_value = DBL_MAX;

        for (Move& m : sorted_moves) {
            engine.push(m);
            double score_value = store_board_values(depth - 1, Color::WHITE, alpha, beta, board_values);
            engine.pop();
            board_values[engine.game_board.zobrist_key][m] = score_value;

            min_move_value = min(min_move_value, score_value);
            beta = min(beta, min_move_value);
            if (beta <= alpha) {
                break;
            }
        }
        return min_move_value;
    }
}

// if (engine.game_board.zobrist_key != starting_zobrist_key) {
//     print_gameboard(engine.game_board);
//     cout << "zobrist key doesn't match for: " << move_to_string(m) << endl;
//     exit(1);
// }

Move MinimaxAI::get_move_iterative_deepening(double time) {
    int zobrist_key_start = engine.game_board.zobrist_key;
    cout << "zobrist_key at start of get_move_iterative_deepening is: " << zobrist_key_start << endl;

    auto start_time = chrono::high_resolution_clock::now();
    auto required_end_time = start_time + chrono::duration<double>(time);

    nodes_visited = 0;


    int color_multiplier = 1;
    if (engine.game_board.turn == Color::BLACK) {
        color_multiplier = -1;
    }


    unordered_map<int, unordered_map<Move, double, MoveHash>> board_values;
    int depth = 1;
    while (chrono::high_resolution_clock::now() <= required_end_time) {
        cout << "Deepening to " << depth << endl;
        store_board_values(depth, engine.game_board.turn, -DBL_MAX, DBL_MAX, board_values);        
        depth++;
    }

    vector<Move> top_level_moves = engine.get_legal_moves();
    Move move_chosen = top_level_moves[0];
    double max_move_value = -DBL_MAX;

    cout << "Went to depth " << depth - 1 << endl;
    cout << "Found " << board_values.size() << " items inside of board_values" << endl;

    if (board_values.find(engine.game_board.zobrist_key) == board_values.end()) {
        cout << "Did not find zobrist key in board_values, this is bad" << endl;
        exit(1);
    }

    auto stored_move_values = board_values[engine.game_board.zobrist_key];
    for (auto move_and_value : stored_move_values) {
        Move m = move_and_value.first;
        double score_value = move_and_value.second * color_multiplier;
        if (score_value > max_move_value) {
            max_move_value = score_value;
            move_chosen = m;
        }
    }

    for (Move& m : top_level_moves) {
        if (stored_move_values.find(m) == stored_move_values.end()) {
            cout << "did not find move " << move_to_string(m) << " in the stored moves. this is probably bad" << endl;
            exit(1);
        }
    }
    string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";
    cout << colorize(AColor::BRIGHT_CYAN, "Minimax AI get_move_iterative_deepening chose move: for " + color + " player with score of " + to_string(max_move_value)) << endl;
    cout << colorize(AColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + " nodes total") << endl;
    cout << colorize(AColor::BRIGHT_BLUE, "Time it was supposed to take: " + to_string(time) + " s") << endl;
    cout << colorize(AColor::BRIGHT_GREEN, "Actual time taken: " + to_string(chrono::duration<double>(chrono::high_resolution_clock::now() - start_time).count()) + " s") << endl;
    cout << "get_move_iterative_deepening zobrist_key at begining: " << zobrist_key_start << ", at end: " << engine.game_board.zobrist_key << endl;
    return move_chosen;
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
    
    if (depth == 0) {
        Color color_perspective = Color::BLACK;
        if (color_multiplier) {
            color_perspective = Color::WHITE;
        }
        return evaluate_board(color_perspective) * color_multiplier;
    }

    if (color_multiplier == 1) {  // Maximizing player
        double max_move_value = -DBL_MAX;
        for (Move& m : moves) {
            engine.push(m);
            double score_value = -1 * get_value(depth - 1, color_multiplier * -1, alpha, beta);
            engine.pop();
            max_move_value = max(max_move_value, score_value);
            alpha = max(alpha, max_move_value);
            if (beta <= alpha) {
                break; // beta cut-off
            }
        }
        return max_move_value;
    } else {  // Minimizing player
        double min_move_value = DBL_MAX;
        for (Move& m : moves) {
            engine.push(m);
            double score_value = -1 * get_value(depth - 1, color_multiplier * -1, alpha, beta);
            engine.pop();
            min_move_value = min(min_move_value, score_value);
            beta = min(beta, min_move_value);
            if (beta <= alpha) {
                break; // alpha cut-off
            }
        }
        return min_move_value;
    }
}

Move MinimaxAI::get_move(int depth) {
    auto start_time = chrono::high_resolution_clock::now();

    nodes_visited = 0;

    int color_multiplier = 1;
    if (engine.game_board.turn == Color::BLACK) {
        color_multiplier = -1;
    }

    Move move_chosen;
    double max_move_value = -DBL_MAX;
    vector<Move> moves = engine.get_legal_moves();
    for (Move& m : moves) {
        engine.push(m);
        double score_value = get_value(depth - 1, color_multiplier * -1, -DBL_MAX, DBL_MAX);
        if (score_value * -1 > max_move_value) {
            max_move_value = score_value * -1;
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



// double MinimaxAI::get_value(int depth, int color_multiplier, double white_highest_val, double black_highest_val) {
//     // if the gamestate is over
//     nodes_visited++;
//     vector<Move> moves = engine.get_legal_moves();
//     GameState state = engine.game_over(moves);

//     if (state == GameState::BLACKWIN) {
//         return ((-DBL_MAX) + 1) * (color_multiplier);
//     }
//     else if (state == GameState::WHITEWIN) {
//         return (DBL_MAX - 1) * (color_multiplier);
//     }
//     else if (state == GameState::DRAW) {
//         return 0;
//     }
    
//     // if we are at max depth
//     if (depth == 0) {
//         Color color_perspective = Color::BLACK;
//         if (color_multiplier) {
//             color_perspective = Color::WHITE;
//         }
//         return evaluate_board(color_perspective) * color_multiplier;
//     }

//     // calculate leaf nodes
//     double max_move_value = -DBL_MAX;
//     for (Move& m : moves) {
//         engine.push(m);
//         double score_value = -1 * get_value(depth - 1, color_multiplier * -1, white_highest_val, black_highest_val);
//         if (score_value > max_move_value) {
//             max_move_value = score_value;
//         }
//         if (color_multiplier) {
//             if (max_move_value > white_highest_val) {
//                 white_highest_val = max_move_value;
//             }
//             // if (-black_highest_val < white_highest_val) {
//             //     engine.pop();
//             //     break;d
//             // }
//         }
//         else {
//             if (max_move_value > black_highest_val) {
//                 black_highest_val = max_move_value;
//             }
//             // if (-black_highest_val < white_highest_val) {
//             //     engine.pop();
//             //     break;
//             // }
//         }
//         engine.pop();
//     }
//     return max_move_value;
// }
