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

        template <Piece piece>
        inline Move* add_move_to_vector(Move* curr_move, ull single_bitboard_from, ull bitboard_to, Color color, bool capture, bool is_laser, ull pieces_lasered_ray) {
            while (bitboard_to) {
                ull single_bitboard_to = utility::bit::lsb_and_pop(bitboard_to);
                Piece piece_captured = Piece::NONE;
                if (capture) {
                    piece_captured = { game_board.get_piece_type_on_bitboard(single_bitboard_to) };
                }

                curr_move->color = color;
                curr_move->piece_type = piece;
                curr_move->from = single_bitboard_from;
                curr_move->to = single_bitboard_to;
                curr_move->capture = piece_captured;
                curr_move->is_laser = is_laser;

                if (is_laser) {
                    curr_move->lasered_pieces = 0;
                    for (const auto& color : color_arr) {
                        ull king_single = game_board.get_pieces_template<Piece::KING>(color) & pieces_lasered_ray;
                        if (king_single) {
                            curr_move->lasered_color[curr_move->lasered_pieces] = color;
                            curr_move->lasered_piece[curr_move->lasered_pieces] = Piece::KING;
                            curr_move->lasered_bitboard[curr_move->lasered_pieces] = king_single;
                            curr_move->lasered_pieces += 1;
                        }

                        ull knights_multi = game_board.get_pieces_template<Piece::KNIGHT>(color) & pieces_lasered_ray;
                        while (knights_multi) {
                            ull single = utility::bit::lsb_and_pop(knights_multi);
                            curr_move->lasered_color[curr_move->lasered_pieces] = color;
                            curr_move->lasered_piece[curr_move->lasered_pieces] = Piece::KNIGHT;
                            curr_move->lasered_bitboard[curr_move->lasered_pieces] = single;
                            curr_move->lasered_pieces += 1;
                        }

                        ull pawns_multi = game_board.get_pieces_template<Piece::PAWN>(color) & pieces_lasered_ray;
                        while (pawns_multi) {
                            ull single = utility::bit::lsb_and_pop(pawns_multi);
                            curr_move->lasered_color[curr_move->lasered_pieces] = color;
                            curr_move->lasered_piece[curr_move->lasered_pieces] = Piece::PAWN;
                            curr_move->lasered_bitboard[curr_move->lasered_pieces] = single;
                            curr_move->lasered_pieces += 1;
                        }

                        ull queen_single = game_board.get_pieces_template<Piece::QUEEN>(color) & pieces_lasered_ray;
                        if (queen_single) {
                            curr_move->lasered_color[curr_move->lasered_pieces] = color;
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
        Move* add_pawn_moves_to_vector(Move*, Color);
        Move* add_knight_moves_to_vector(Move*, Color);
        Move* add_queen_moves_to_vector(Move*, Color);
        Move* add_king_moves_to_vector(Move*, Color);
        Move* add_rook_moves_to_vector(Move*, Color);

        // helpers for move generation
        ull get_laser_attack(ull);
        ull get_straight_attacks(ull);
};
} // end namespace ShumiChess