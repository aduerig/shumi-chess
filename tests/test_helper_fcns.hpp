#include <gtest/gtest.h>

#include "gameboard.hpp"
#include "globals.hpp"

namespace ShumiChess {
bool operator==(const ShumiChess::GameBoard& a, const ShumiChess::GameBoard& b) {
    return (a.black_pawns == b.black_pawns &&
            a.white_pawns == b.white_pawns &&
            a.black_rooks == b.black_rooks &&
            a.white_rooks == b.white_rooks &&
            a.black_knights == b.black_knights &&
            a.white_knights == b.white_knights &&
            a.black_bishops == b.black_bishops &&
            a.white_bishops == b.white_bishops &&
            a.black_queens == b.black_queens &&
            a.white_queens == b.white_queens &&
            a.black_king == b.black_king &&
            a.white_king == b.white_king &&
            a.turn == b.turn &&
            a.black_castle == b.black_castle &&
            a.white_castle == b.white_castle &&
            a.en_passant == b.en_passant &&
            a.halfmove == b.halfmove &&
            a.fullmove == b.fullmove);
}
}