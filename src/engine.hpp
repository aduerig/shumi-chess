#pragma once

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

        ull& access_piece_of_color(ShumiChess::Piece, ShumiChess::Color);

        void apply_en_passant_checks(const Move&);
        void apply_castling_changes(const Move&);

        vector<Move> get_legal_moves();
        vector<Move> get_pawn_moves(Color player_Color);
        vector<Move> get_knight_moves(Color player_Color);
        vector<Move> get_bishop_moves(Color player_Color);
        vector<Move> get_queen_moves(Color player_Color);
        vector<Move> get_king_moves(Color player_Color);
        vector<Move> get_rook_moves(Color player_Color);
};
} // end namespace ShumiChess