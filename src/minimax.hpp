#pragma once

#include <spp.h>
#include <bitset>

#include <globals.hpp>
#include <engine.hpp>
#include <utility.hpp>
#include <algorithm>
#include <cfloat>



class RandomAI {
public:
    ShumiChess::Engine engine;
    spp::sparse_hash_map<ull, int> piece_and_values;

    RandomAI(ShumiChess::Engine&);
    int rand_int(int, int);
    ShumiChess::Move& get_move(vector<ShumiChess::Move>&);
};


struct MoveBoardValueDepth {
    ShumiChess::Move move;
    double board_value;
    int depth;
};

class MinimaxAI {
public:
    int nodes_visited = 0;
    int max_depth = 0;

    ShumiChess::Engine& engine;
    std::array<ShumiChess::Piece, 6> all_pieces = {
        ShumiChess::Piece::PAWN,
        ShumiChess::Piece::KNIGHT,
        ShumiChess::Piece::ROOK,
        ShumiChess::Piece::QUEEN,
        ShumiChess::Piece::KING,
    };

    std::array<double, 6> all_piece_values = {1, 3, 5, 8, 0};
    spp::sparse_hash_map<uint64_t, std::string> seen_zobrist;

    std::array<std::array<double, 8>, 8> pawn_values = {{
        {5, 5, 7, 7, 0, 0, 0, 0},
        {4, 6, 6, 8, 8, 8, 8, 0},
        {5, 5, 7, 7, 7, 7, 8, 0},
        {4, 6, 6, 6, 6, 6, 8, 0},
        {5, 5, 5, 5, 5, 5, 8, 7},
        {4, 4, 4, 4, 4, 7, 6, 7},
        {3, 3, 3, 3, 6, 5, 6, 5},
        {2, 2, 2, 5, 4, 5, 4, 5},
    }};

    double black_pawn_value_lookup[64];
    double white_pawn_value_lookup[64];

    ShumiChess::Move capture_moves_internal[256];

    MinimaxAI(ShumiChess::Engine&);

    inline double game_over_value(ShumiChess::GameState state, ShumiChess::Color color) {
        double end_value = 0;
        if (state == ShumiChess::GameState::BLACKWIN) {
            end_value = engine.game_board.turn == ShumiChess::WHITE ? -DBL_MAX / 2 : DBL_MAX / 2;
        }
        else if (state == ShumiChess::GameState::WHITEWIN) {
            end_value = engine.game_board.turn == ShumiChess::BLACK ? -DBL_MAX / 2 : DBL_MAX / 2;
        }
        return end_value;
    };


    inline ShumiChess::LegalMoves get_capture_moves(ShumiChess::LegalMoves legal_moves) {
        int move_counter = 0;

        // for (int i = 0; i < legal_moves.num_moves; i++) {
        //     capture_moves_internal[move_counter] = legal_moves.moves[i];
        //     move_counter += min(1, 5 - (int) legal_moves.moves[i].piece_captured);
        // }

        for (int i = 0; i < legal_moves.num_moves; i++) {
            if (legal_moves.moves[i].piece_captured != ShumiChess::Piece::NONE) {
                capture_moves_internal[move_counter] = legal_moves.moves[i];
                move_counter += 1;
            }
        }
        return {capture_moves_internal, move_counter};
    };

    double Quiesce(int, int, double, double);


    template <ShumiChess::Piece piece_type>
    inline double evaluate_board(ShumiChess::Color for_color, ShumiChess::LegalMoves legal_moves) {
        double board_val_adjusted = 0;
        double* pawn_value_lookup = white_pawn_value_lookup;
        for (const auto& color : ShumiChess::color_arr) {
            double board_val = 0;
            double piece_value = all_piece_values[(int)piece_type];
            
            ull pieces_bitboard = engine.game_board.get_pieces<piece_type>(color);

            board_val += piece_value * bits_in(pieces_bitboard);

            if constexpr (piece_type == ShumiChess::Piece::PAWN) {
                while (pieces_bitboard) {
                    int square = utility::bit::lsb_and_pop_to_square(pieces_bitboard);
                    board_val += pawn_value_lookup[square];
                }
            }
            if (color != for_color) {
                board_val *= -1;
            }
            board_val_adjusted += board_val;
            pawn_value_lookup = black_pawn_value_lookup;
        }
        return board_val_adjusted;
    }

    inline double evaluate_board(ShumiChess::Color color, ShumiChess::LegalMoves legal_moves) {
        double avail_move_bonus = (legal_moves.num_moves / 70.0);
        return evaluate_board<ShumiChess::Piece::PAWN>(engine.game_board.turn, legal_moves) + evaluate_board<ShumiChess::Piece::ROOK>(engine.game_board.turn, legal_moves) + evaluate_board<ShumiChess::Piece::KNIGHT>(engine.game_board.turn, legal_moves) + evaluate_board<ShumiChess::Piece::QUEEN>(engine.game_board.turn, legal_moves) + avail_move_bonus;
    }

    inline int bits_in(ull bitboard) {
        auto bs = bitset<64>(bitboard);
        return (int) bs.count();
        
        // unsigned int c; // c accumulates the total bits set in v
        // option 1, for at most 14-bit values in v:
        // c = (bitboard * 0x200040008001ULL & 0x111111111111111ULL) % 0xf;
        // return c;

        // return std::popcount(bitboard);
    }

    ShumiChess::MoveAndBoardValue store_board_values_negamax(int, int, double, double, spp::sparse_hash_map<uint64_t, spp::sparse_hash_map<ShumiChess::Move, double, utility::representation::MoveHash>> &, spp::sparse_hash_map<int, MoveBoardValueDepth> &, bool);
    ShumiChess::Move get_move_iterative_deepening(double);
};