#pragma once

#include <unordered_map>

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


// ? can use inline here, but it complicates the build. defining in gloabls.cpp
extern ull a_row;
extern std::unordered_map<int, ull> rank_masks;

} // end namespace ShumiChess
