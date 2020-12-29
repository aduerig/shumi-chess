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

inline ull a_row = 1 << 0 | 1 << 1 | 1 << 2 | 1 << 3 |
            1 << 4 | 1 << 5 | 1 << 6 | 1 << 7;
inline std::unordered_map<int, ull> rank_masks = {
    {1, a_row},
    {2, a_row << 8},
    {3, a_row << 16},
    {4, a_row << 24},
    {5, a_row << 32},
    {6, a_row << 40},
    {7, a_row << 48},
    {8, a_row << 56}
};

} // end namespace ShumiChess
