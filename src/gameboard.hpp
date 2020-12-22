#pragma once

#include <string>

#include "globals.hpp"

namespace ShumiChess {
    // TODO think about copy and move constructors.
    // Will we ever want to copy?
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
            Color turn;

            // Constructors
            explicit GameBoard();
            explicit GameBoard(const std::string&);

            // Operators
            //TODO: If this is only useful in testing should either:
            //1. Preprocessor defs to only compile it in debug
            //2. Implement a GTest mock gameboard class for this functionality
            friend bool operator==(const GameBoard&, const GameBoard&);

            // Member methods
            const std::string to_fen();
    };

    //TODO
    // See above on ==
    bool operator==(const GameBoard& a, const GameBoard& b) {
        return this->black_pawns   == that.black_pawns &&
                       this->white_pawns   == that.white_pawns &&
                       this->black_rooks   == that.black_rooks &&
                       this->white_rooks   == that.white_rooks &&
                       this->black_knights == that.black_knights &&
                       this->white_knights == that.white_knights &&
                       this->black_bishops == that.black_bishops &&
                       this->white_bishops == that.white_bishops &&
                       this->black_queens  == that.black_queens &&
                       this->white_queens  == that.white_queens &&
                       this->black_king    == that.black_king &&
                       this->white_king    == that.white_king &&
                       this->turn          == that.turn;
    }
}
