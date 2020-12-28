#pragma once

typedef unsigned long long ull;

namespace ShumiChess {
enum Color {
    WHITE,
    BLACK
};

enum Piece {
    PAWN,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING
};

// TODO think about if this is the right way to represent
struct Move {
    ull from; // square
    ull to; // square
    Piece piece_type;
    Color color; 
};
} // end namespace ShumiChess
