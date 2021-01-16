#pragma once

#include <cinttypes>
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
    uint8_t black_castle = 0b00000011;
    uint8_t white_castle = 0b00000011;
    ull en_passent = 0;
};


// ? can use inline here, but it complicates the build. defining in globals.cpp
extern ull a_row;
extern ull a_col;
extern std::unordered_map<int, ull> rank_masks;
extern std::unordered_map<char, ull> col_masks;
} // end namespace ShumiChess
