#pragma once

#include <vector>

#include "gameboard.hpp"

using namespace std;

namespace ShumiChess {
class Engine {
    public:
        // Members
        GameBoard game_board;
    
        // Constructors
        //TODO Should the engine be tied to a single boardstate
        //Should engine functions act independently and take board objects?
        explicit Engine();
        explicit Engine(const string&);

        // Member methods
        void reset_engine();

        void push(Move move);
        void pop();

        vector<Move> get_legal_moves();
        vector<Move> get_pawn_moves(Color player_Color);
        vector<Move> get_knight_moves(Color player_Color);
        vector<Move> get_bishop_moves(Color player_Color);
        vector<Move> get_queen_moves(Color player_Color);
        vector<Move> get_king_moves(Color player_Color);
        vector<Move> get_rook_moves(Color player_Color);
};
} // end namespace ShumiChess