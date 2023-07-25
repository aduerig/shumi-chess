#include <float.h>
#include <bitset>
#include <iomanip>
#include <sstream>
#include <locale>

#include "minimax.hpp"
#include "utility.hpp"

using namespace std;
using namespace ShumiChess;

MinimaxAI::MinimaxAI(Engine& e) : engine(e) { }


int MinimaxAI::evaluate_board() {
    return evaluate_board(engine.game_board.turn);
}

int MinimaxAI::bits_in(ull bitboard)
{
    auto bs = std::bitset<64>(bitboard);
    return (int) bs.count();
}

template<class T>
std::string format_with_commas(T value) {
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << std::fixed << value;
    return ss.str();
}

int MinimaxAI::evaluate_board(Color color) {
    Color opposite_turn = utility::representation::get_opposite_color(color);

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



double MinimaxAI::store_board_values(int depth, int color_multiplier, double alpha, double beta, unordered_map<int, double> board_values) {
    nodes_visited++;
    vector<ShumiChess::Move> moves = engine.get_legal_moves();
    ShumiChess::GameState state = engine.game_over(moves);
    
    if (state == ShumiChess::GameState::BLACKWIN) {
        return (-DBL_MAX + 1) * (color_multiplier);
    }
    else if (state == ShumiChess::GameState::WHITEWIN) {
        return (DBL_MAX - 1) * (color_multiplier);
    }
    else if (state == ShumiChess::GameState::DRAW) {
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
            max_move_value = std::max(max_move_value, score_value);
            alpha = std::max(alpha, max_move_value);
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
            min_move_value = std::min(min_move_value, score_value);
            beta = std::min(beta, min_move_value);
            if (beta <= alpha) {
                break; // alpha cut-off
            }
        }
        return min_move_value;
    }
}


Move MinimaxAI::get_move_iterative_deepening(double time) {
    auto start_time = std::chrono::high_resolution_clock::now();
    auto required_end_time = start_time + std::chrono::duration<double>(time);

    nodes_visited = 0;


    int color_multiplier = 1;
    if (engine.game_board.turn == Color::BLACK) {
        color_multiplier = -1;
    }

    unordered_map<int, double> board_values;
    int depth = 1;
    while (std::chrono::high_resolution_clock::now() <= required_end_time) {
        store_board_values(depth, color_multiplier * -1, -DBL_MAX, DBL_MAX, board_values);        
        depth++;
    }

    vector<ShumiChess::Move> top_level_moves = engine.get_legal_moves();
    Move move_chosen = top_level_moves[0];
    double max_move_value = -DBL_MAX;

    for (Move& m : top_level_moves) {
        engine.push(m);
        // !TODO is this the proper way to search an unordered map?
        if (board_values.find(engine.game_board.turn) != board_values.end()) {
            // int board_hash = engine.game_board;
            int board_hash = 0;
            double score_value = color_multiplier * board_values[board_hash];
            if (score_value > max_move_value) {
                max_move_value = score_value;
                move_chosen = m;
            }
        }
        engine.pop();
    }
    std::string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";
    cout << utility::colorize(utility::AnsiColor::BRIGHT_CYAN, "Minimax AI chose move: for " + color + " player") << endl;
    cout << utility::colorize(utility::AnsiColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + " nodes total") << endl;
    cout << utility::colorize(utility::AnsiColor::BRIGHT_BLUE, "Time it was supposed to take: " + std::to_string(time) + " s") << endl;
    cout << utility::colorize(utility::AnsiColor::BRIGHT_GREEN, "Actual time taken: " + std::to_string(std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_time).count()) + " s") << endl;
    return move_chosen;
}


// === straight forward minimax below ===
double MinimaxAI::get_value(int depth, int color_multiplier, double alpha, double beta) {
    nodes_visited++;
    vector<ShumiChess::Move> moves = engine.get_legal_moves();
    ShumiChess::GameState state = engine.game_over(moves);
    
    if (state == ShumiChess::GameState::BLACKWIN) {
        return (-DBL_MAX + 1) * (color_multiplier);
    }
    else if (state == ShumiChess::GameState::WHITEWIN) {
        return (DBL_MAX - 1) * (color_multiplier);
    }
    else if (state == ShumiChess::GameState::DRAW) {
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
            max_move_value = std::max(max_move_value, score_value);
            alpha = std::max(alpha, max_move_value);
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
            min_move_value = std::min(min_move_value, score_value);
            beta = std::min(beta, min_move_value);
            if (beta <= alpha) {
                break; // alpha cut-off
            }
        }
        return min_move_value;
    }
}

Move MinimaxAI::get_move(int depth) {
    auto start_time = std::chrono::high_resolution_clock::now();

    nodes_visited = 0;

    int color_multiplier = 1;
    if (engine.game_board.turn == Color::BLACK) {
        color_multiplier = -1;
    }

    Move move_chosen;
    double max_move_value = -DBL_MAX;
    vector<ShumiChess::Move> moves = engine.get_legal_moves();
    for (Move& m : moves) {
        engine.push(m);
        double score_value = get_value(depth - 1, color_multiplier * -1, -DBL_MAX, DBL_MAX);
        if (score_value * -1 > max_move_value) {
            max_move_value = score_value * -1;
            move_chosen = m;
        }
        engine.pop();
    }
    
    std::string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";
    cout << utility::colorize(utility::AnsiColor::BRIGHT_CYAN, "Minimax AI chose move: for " + color + " player") << endl;
    cout << utility::colorize(utility::AnsiColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + " nodes total") << endl;

    std::chrono::duration<double> total_time = std::chrono::high_resolution_clock::now() - start_time;
    cout << utility::colorize(utility::AnsiColor::BRIGHT_GREEN, "Total time taken for get_move(): " + std::to_string(total_time.count()) + " s") << endl;

    return move_chosen;
}

Move MinimaxAI::get_move() {
    int default_depth = 7;
    cout << utility::colorize(utility::AnsiColor::BRIGHT_GREEN, "Called MinimaxAI::get_move() with no arguments, using default depth: " + std::to_string(default_depth)) << endl;
    return get_move(default_depth);
}



// double MinimaxAI::get_value(int depth, int color_multiplier, double white_highest_val, double black_highest_val) {
//     // if the gamestate is over
//     nodes_visited++;
//     vector<ShumiChess::Move> moves = engine.get_legal_moves();
//     ShumiChess::GameState state = engine.game_over(moves);

//     if (state == ShumiChess::GameState::BLACKWIN) {
//         return ((-DBL_MAX) + 1) * (color_multiplier);
//     }
//     else if (state == ShumiChess::GameState::WHITEWIN) {
//         return (DBL_MAX - 1) * (color_multiplier);
//     }
//     else if (state == ShumiChess::GameState::DRAW) {
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
