#pragma once

#include <optional>
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
    KING,
    NONE
};

// TODO think about if this is the right way to represent
struct Move {
    Color color;
    Piece piece_type;
    ull from; // square
    ull to; // square
    std::optional<Piece> capture;
    std::optional<Piece> promotion;
    //castling info
    ull en_passent {};
};


// ? can use inline here, but it complicates the build. defining in gloabls.cpp
extern ull a_row;
extern std::unordered_map<int, ull> rank_masks;

} // end namespace ShumiChess
