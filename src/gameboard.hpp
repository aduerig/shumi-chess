#pragma once

#include <string>

#include "globals.hpp"

namespace ShumiChess {
// TODO think about copy and move constructors.
// ? Will we ever want to copy?
class GameBoard {
    public:
        // Piece locations
        ull black_pawns {};
        ull white_pawns {};

        ull black_rooks {};
        ull white_rooks {};

        ull black_knights {};
        ull white_knights {};

        ull black_bishops {};
        ull white_bishops {};

        ull black_queens {};
        ull white_queens {};

        ull black_king {};
        ull white_king {};

        // other information about the board state
        Color turn;

        //1<<1 for queenside, 1<<0 for kingside
        uint8_t black_castle = 0b00000000;
        uint8_t white_castle = 0b00000000;

        //square behind pawn after moving 2
        //0 is impossible state in conventional game
        ull en_passant; 

        // move clock
        uint halfmove;
        uint fullmove;

        // Constructors
        explicit GameBoard();
        explicit GameBoard(const std::string&);

        // Member methods
        const std::string to_fen();
};
} // end namespace ShumiChess
