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

        void add_move_to_vector(vector<Move>&, ull, ull, Piece, Color, bool, bool, ull);

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