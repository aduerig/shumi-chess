#pragma once

#include <cinttypes>
#include <optional>
#include <unordered_map>
#include <vector>
#include <ostream>
#include <iostream>
#include <array>

typedef unsigned long long ull;

namespace ShumiChess {
enum Color {
    WHITE,
    BLACK
};

enum Piece {
    NONE,
    PAWN,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING
};

// TODO think about if this is the right way to represent
struct Move {
    Color color;
    Piece piece_type;
    ull from; // bitboard
    ull to; // bitboard
    Piece capture = Piece::NONE;
    Piece promotion = Piece::NONE;
    uint8_t black_castle = 0b00000011;
    uint8_t white_castle = 0b00000011;
    ull en_passant = 0;
    bool is_en_passent_capture = false;
    bool is_castle_move = false;
};

// ? is this best way to number
// ? Should we try to make most checked things = 0 for magical compiler iszero optimizations? (applies to piece enum as well)
enum GameState {
    INPROGRESS = -1,
    WHITEWIN = 0,
    DRAW = 1,
    BLACKWIN = 2
};


// ? maybe can use inline here for externs? but it complicates the build. defining in globals.cpp

enum Row {
    ROW_1 = 0,
    ROW_2 = 1,
    ROW_3 = 2,
    ROW_4 = 3,
    ROW_5 = 4,
    ROW_6 = 5,
    ROW_7 = 6,
    ROW_8 = 7
};

enum Col {
    COL_A = 0,
    COL_B = 1,
    COL_C = 2,
    COL_D = 3,
    COL_E = 4,
    COL_F = 5,
    COL_G = 6,
    COL_H = 7
};

// TODO move all this to movegen

extern ull a_row;
extern ull a_col;
extern std::vector<ull> row_masks;
extern std::vector<ull> col_masks;

extern std::unordered_map<int, ull> down_right_diagonals;
extern std::unordered_map<int, ull> down_left_diagonals;

extern std::array<ull, 64> square_to_y;
extern std::array<ull, 64> square_to_x;

extern std::array<ull, 65> north_east_square_ray;
extern std::array<ull, 65> north_west_square_ray;
extern std::array<ull, 65> south_east_square_ray;
extern std::array<ull, 65> south_west_square_ray;

extern std::array<ull, 65> north_square_ray;
extern std::array<ull, 65> south_square_ray;
extern std::array<ull, 65> east_square_ray;
extern std::array<ull, 65> west_square_ray;

void initialize_rays();

extern std::vector<Piece> promotion_values;

} // end namespace ShumiChess
