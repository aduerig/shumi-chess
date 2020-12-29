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

        //square behind enpassantable pawn after moving
        //0 is impossible state in conventional game
        // ? is this right way to represent this
        // ? do we care about non standard gameboards / moves
        // ? would one consider an extra bit for if we should look at this val
        // ? what about an std::optional https://stackoverflow.com/questions/23523184/overhead-of-stdoptionalt
        ull en_passant {}; 

        // move clock
        uint8_t halfmove;
        uint8_t fullmove;

        // Constructors
        explicit GameBoard();
        explicit GameBoard(const std::string&);

        // Member methods
        const std::string to_fen();
        
        ull get_piece(Color);
        ull get_piece(Piece);
        ull get_piece(Color, Piece);
};
} // end namespace ShumiChess
