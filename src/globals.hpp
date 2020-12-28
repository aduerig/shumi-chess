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
    int from; // square
    int to; // square
    Color color; 
    Piece piece_type;
};
} // end namespace ShumiChess
