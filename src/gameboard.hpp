#pragma once

#include <string>

#include "typedef.hpp"

namespace ShumiChess {
    // TODO think about best place to put color enum 
    enum color
    {   
        white, 
        black
    };

    // TODO think about copy and move constructors.
    class GameBoard {
        public:
            // Piece locations
            ull black_pawns;
            ull white_pawns;

            ull black_rooks;
            ull white_rooks;

            ull black_knights;
            ull white_knights;

            ull black_bishops;
            ull white_bishops;

            ull black_queens;
            ull white_queens;

            ull black_king;
            ull white_king;

            // other information about the board state
            color turn;

            // Constructors
            explicit GameBoard();
            explicit GameBoard(const std::string&);

            // Member methods
            const std::string to_fen();
    };
}
