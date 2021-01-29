#pragma once

#include <cassert>
#include <iostream>
#include <stack>
#include <vector>

#include "globals.hpp"
#include "gameboard.hpp"
#include "utility.hpp"

using namespace std;

namespace ShumiChess {
class Engine {
    public:
        // Members
        GameBoard game_board;

        std::stack<Move> move_history;
        std::stack<int> halfway_move_history;
        //TODO This way of tracking is inconsistent with gameboard, think over. Requires splitting and merging oof
        std::stack<uint8_t> castle_opportunity_history; //Bits right to left: white kingside, white queenside, black kingside, black queenside
    
        // Constructors
        //? Should the engine be tied to a single boardstate
        //? Should engine functions act independently and take board objects?
        explicit Engine();
        explicit Engine(const string&);

        // Member methods
        void reset_engine();

        void push(const Move&);
        void pop();
        GameState game_over();

        ull& access_piece_of_color(Piece, Color);

        void apply_en_passant_checks(const Move&);
        void apply_castling_changes(const Move&);

        void add_move_to_vector(vector<Move>&, ull, ull, Piece, Color, bool, bool, ull, bool);
        // ? maybe shouldn't be here?
        Piece get_piece_on_bitboard(ull);

        vector<Move> get_legal_moves();
        vector<Move> get_psuedo_legal_moves(Color);
        void add_pawn_moves_to_vector(vector<Move>&, Color);
        void add_knight_moves_to_vector(vector<Move>&, Color);
        void add_bishop_moves_to_vector(vector<Move>&, Color);
        void add_queen_moves_to_vector(vector<Move>&, Color);
        void add_king_moves_to_vector(vector<Move>&, Color);
        void add_rook_moves_to_vector(vector<Move>&, Color);

        // helpers for move generation
        ull get_diagonal_attacks(ull);
        ull get_straight_attacks(ull);
};
} // end namespace ShumiChess