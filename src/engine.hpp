#pragma once

#include <cassert>
#include <iostream>
#include <stack>
#include <vector>

#include <spp.h>

#include "globals.hpp"
#include "gameboard.hpp"
#include "move_tables.hpp"
#include "utility.hpp"

using namespace std;

namespace ShumiChess {
class Engine {
    public:
        GameBoard game_board;

        std::stack<Move> move_history;
    
        explicit Engine();
        explicit Engine(const string&);

        void reset_engine();

        void push(const Move&);
        void pop();

        GameState game_over();
        GameState game_over(LegalMoves);

        ull& access_piece_of_color(Piece, Color);
        ull all_pieces_cache;

        template <Piece piece, Color color, bool is_capture, bool is_laser>
        inline Move* add_move_to_vector(Move* curr_move, ull single_bitboard_from, ull bitboard_to, ull pieces_lasered_ray) {
            while (bitboard_to) {
                ull single_bitboard_to = utility::bit::lsb_and_pop(bitboard_to);
                Piece piece_captured = Piece::NONE;

                if constexpr (is_capture) { 
                    Piece piece_captured = { game_board.get_piece_type_on_bitboard(single_bitboard_to) };
                }

                curr_move->color = color;
                curr_move->piece_type = piece;
                curr_move->from = single_bitboard_from;
                curr_move->to = single_bitboard_to;
                curr_move->is_capture = piece_captured;
                curr_move->is_laser = is_laser;

                if constexpr (is_laser) { 
                    curr_move->lasered_pieces = 0;
                    for (const auto& color_iter : color_arr) {
                        ull king_single = game_board.get_pieces<Piece::KING>(color_iter) & pieces_lasered_ray;
                        if (king_single) {
                            curr_move->lasered_color[curr_move->lasered_pieces] = color_iter;
                            curr_move->lasered_piece[curr_move->lasered_pieces] = Piece::KING;
                            curr_move->lasered_bitboard[curr_move->lasered_pieces] = king_single;
                            curr_move->lasered_pieces += 1;
                        }

                        ull knights_multi = game_board.get_pieces<Piece::KNIGHT>(color_iter) & pieces_lasered_ray;
                        while (knights_multi) {
                            ull single = utility::bit::lsb_and_pop(knights_multi);
                            curr_move->lasered_color[curr_move->lasered_pieces] = color_iter;
                            curr_move->lasered_piece[curr_move->lasered_pieces] = Piece::KNIGHT;
                            curr_move->lasered_bitboard[curr_move->lasered_pieces] = single;
                            curr_move->lasered_pieces += 1;
                        }

                        ull pawns_multi = game_board.get_pieces<Piece::PAWN>(color_iter) & pieces_lasered_ray;
                        while (pawns_multi) {
                            ull single = utility::bit::lsb_and_pop(pawns_multi);
                            curr_move->lasered_color[curr_move->lasered_pieces] = color_iter;
                            curr_move->lasered_piece[curr_move->lasered_pieces] = Piece::PAWN;
                            curr_move->lasered_bitboard[curr_move->lasered_pieces] = single;
                            curr_move->lasered_pieces += 1;
                        }

                        ull queen_single = game_board.get_pieces<Piece::QUEEN>(color_iter) & pieces_lasered_ray;
                        if (queen_single) {
                            curr_move->lasered_color[curr_move->lasered_pieces] = color_iter;
                            curr_move->lasered_piece[curr_move->lasered_pieces] = Piece::QUEEN;
                            curr_move->lasered_bitboard[curr_move->lasered_pieces] = queen_single;
                            curr_move->lasered_pieces += 1;
                        }
                    }
                }
                curr_move++;
            }
            return curr_move;
        };

        Move start_of_moves[256];

        spp::sparse_hash_map<uint64_t, int> count_zobrist_states;
        bool three_fold_rep_draw {false};


        LegalMoves get_legal_moves();

        template <Color color>
        LegalMoves get_legal_moves();

        template <Color color>
        Move* add_pawn_moves_to_vector(Move*);

        template <Color color>
        Move* add_knight_moves_to_vector(Move*);

        template <Color color>
        Move* add_queen_moves_to_vector(Move*);

        template <Color color>
        Move* add_king_moves_to_vector(Move*);

        template <Color color>
        Move* add_rook_moves_to_vector(Move*);

        // helpers for move generation
        ull get_laser_attack(ull);
        ull get_straight_attacks(ull);
};
} // end namespace ShumiChess