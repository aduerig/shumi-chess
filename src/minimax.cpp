#include <float.h>
#include <bitset>
#include <iomanip>
#include <locale>

#include "minimax.hpp"

using namespace std;
using namespace ShumiChess;

MinimaxAI::MinimaxAI(Engine& e) : engine(e) { }

int MinimaxAI::evaluate_board() {
    return evaluate_board(engine.game_board.turn);
}

int MinimaxAI::bits_in(ull bitboard)
{
    auto bs = std::bitset<64>(bitboard);
    return bs.count();
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
        //     utility::bit::lsb_and_pop(piece_bitboard);
        //     total_board_val += piece_value;
        // }
    }
    return total_board_val;
}

double MinimaxAI::get_value(int depth, int color_multiplier, double white_highest_val, double black_highest_val) {
    // if the gamestate is over
    nodes_visited++;
    vector<ShumiChess::Move> moves = engine.get_legal_moves();
    ShumiChess::GameState state = engine.game_over(moves);

    if (state == ShumiChess::GameState::BLACKWIN) {
        return ((-DBL_MAX) + 1) * (color_multiplier);
    }
    else if (state == ShumiChess::GameState::WHITEWIN) {
        return (DBL_MAX - 1) * (color_multiplier);
    }
    else if (state == ShumiChess::GameState::DRAW) {
        return 0;
    }
    
    // if we are at max depth
    if (depth == 0) {
        Color color_perspective = Color::BLACK;
        if (color_multiplier) {
            color_perspective = Color::WHITE;
        }
        return evaluate_board(color_perspective) * color_multiplier;
    }

    // calculate leaf nodes
    double max_move_value = -DBL_MAX;
    for (Move& m : moves) {
        engine.push(m);
        double score_value = -1 * get_value(depth - 1, color_multiplier * -1, white_highest_val, black_highest_val);
        if (score_value > max_move_value) {
            max_move_value = score_value;
        }
        if (color_multiplier) {
            if (max_move_value > white_highest_val) {
                white_highest_val = max_move_value;
            }
            // if (-black_highest_val < white_highest_val) {
            //     engine.pop();
            //     break;
            // }
        }
        else {
            if (max_move_value > black_highest_val) {
                black_highest_val = max_move_value;
            }
            // if (-black_highest_val < white_highest_val) {
            //     engine.pop();
            //     break;
            // }
        }
        engine.pop();
    }
    return max_move_value;
}

template<class T>
std::string format_with_commas(T value)
{
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << std::fixed << value;
    return ss.str();
}

Move MinimaxAI::get_move(int depth) {
    if (depth < 1) {
        cout << "Cannot have depth be lower than 1 for minimax_ai" << endl;
        assert(false);
    }

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
        double score_value = get_value(depth - 1, color_multiplier * -1, -DBL_MAX, -DBL_MAX);
        if (score_value * -1 > max_move_value) {
            max_move_value = score_value * -1;
            move_chosen = m;
        }
        engine.pop();
    }
    
    cout << "visited: " << format_with_commas(nodes_visited) << " nodes total" << endl;
    return move_chosen;
}

Move MinimaxAI::get_move() {
    return get_move(4);
}







/// random isn't really used here ///
RandomAI::RandomAI(Engine& e) {
    engine = e;
    piece_and_values = {
        {engine.game_board.get_pieces(engine.game_board.turn, Piece::PAWN), 1},
        {engine.game_board.get_pieces(engine.game_board.turn, Piece::ROOK), 5},
        {engine.game_board.get_pieces(engine.game_board.turn, Piece::KNIGHT), 3},
        {engine.game_board.get_pieces(engine.game_board.turn, Piece::BISHOP), 3},
        {engine.game_board.get_pieces(engine.game_board.turn, Piece::QUEEN), 8},
    };
}

int RandomAI::rand_int(int min, int max) {
    return min + (rand() % static_cast<int>(max - min + 1));
}

Move& RandomAI::get_move(vector<Move>& moves) {
    int index = rand_int(0, moves.size() - 1);
    return moves[index];
}