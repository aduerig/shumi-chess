#pragma once

#include <cassert>
#include <iostream>
#include <stack>
#include <vector>

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
        GameState game_over(vector<Move>&);

        ull& access_piece_of_color(Piece, Color);

        inline void add_move_to_vector(vector<Move>& moves, ull single_bitboard_from, ull bitboard_to, Piece piece, Color color, bool capture, bool is_laser, ull pieces_lasered_ray) {
            while (bitboard_to) {
                ull single_bitboard_to = utility::bit::lsb_and_pop(bitboard_to);
                Piece piece_captured = Piece::NONE;
                if (capture) {
                    piece_captured = { game_board.get_piece_type_on_bitboard(single_bitboard_to) };
                }

                Move new_move;
                new_move.color = color;
                new_move.piece_type = piece;
                new_move.from = single_bitboard_from;
                new_move.to = single_bitboard_to;
                new_move.capture = piece_captured;
                new_move.is_laser = is_laser;

                if (is_laser) {
                    for (const auto& color : color_arr) {
                        ull king_single = game_board.get_pieces_template<Piece::KING>(color) & pieces_lasered_ray;
                        if (king_single) {
                            new_move.lasered_color[new_move.lasered_pieces] = color;
                            new_move.lasered_piece[new_move.lasered_pieces] = Piece::KING;
                            new_move.lasered_bitboard[new_move.lasered_pieces] = king_single;
                            new_move.lasered_pieces += 1;
                        }

                        ull knights_multi = game_board.get_pieces_template<Piece::KNIGHT>(color) & pieces_lasered_ray;
                        while (knights_multi) {
                            ull single = utility::bit::lsb_and_pop(knights_multi);
                            new_move.lasered_color[new_move.lasered_pieces] = color;
                            new_move.lasered_piece[new_move.lasered_pieces] = Piece::KNIGHT;
                            new_move.lasered_bitboard[new_move.lasered_pieces] = single;
                            new_move.lasered_pieces += 1;
                        }

                        ull pawns_multi = game_board.get_pieces_template<Piece::PAWN>(color) & pieces_lasered_ray;
                        while (pawns_multi) {
                            ull single = utility::bit::lsb_and_pop(pawns_multi);
                            new_move.lasered_color[new_move.lasered_pieces] = color;
                            new_move.lasered_piece[new_move.lasered_pieces] = Piece::PAWN;
                            new_move.lasered_bitboard[new_move.lasered_pieces] = single;
                            new_move.lasered_pieces += 1;
                        }

                        ull queen_single = game_board.get_pieces_template<Piece::QUEEN>(color) & pieces_lasered_ray;
                        if (queen_single) {
                            new_move.lasered_color[new_move.lasered_pieces] = color;
                            new_move.lasered_piece[new_move.lasered_pieces] = Piece::QUEEN;
                            new_move.lasered_bitboard[new_move.lasered_pieces] = queen_single;
                            new_move.lasered_pieces += 1;
                        }
                    }
                }
                moves.emplace_back(new_move);
            }
        };

        vector<Move> get_legal_moves();
        void add_pawn_moves_to_vector(vector<Move>&, Color);
        void add_knight_moves_to_vector(vector<Move>&, Color);
        void add_queen_moves_to_vector(vector<Move>&, Color);
        void add_king_moves_to_vector(vector<Move>&, Color);
        void add_rook_moves_to_vector(vector<Move>&, Color);

        // helpers for move generation
        ull get_laser_attack(ull);
        ull get_straight_attacks(ull);
};
} // end namespace ShumiChess