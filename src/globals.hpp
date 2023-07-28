#pragma once

#include <cinttypes>
#include <optional>
#include <unordered_map>
#include <vector>
#include <ostream>
#include <iostream>
#include <array>
#include <cstdlib>

typedef unsigned long long ull;

namespace ShumiChess {
enum Color {
    WHITE = 0,
    BLACK,
};

enum Piece {
    PAWN = 0,
    ROOK,
    KNIGHT,
    QUEEN,
    KING,
    NONE,
};

// TODO think about if this is the right way to represent
struct Move {
    Color color;
    Piece piece_type;
    ull from; // bitboard
    ull to; // bitboard
    Piece capture = Piece::NONE;
    bool is_laser = false;

    uint8_t lasered_pieces = 0;
    ShumiChess::Color lasered_color[7];
    ShumiChess::Piece lasered_piece[7];
    ull lasered_bitboard[7];

    // std::vector<std::tuple<Color, Piece, ull>> pieces_lasered;

    bool operator==(const Move &other) const {
        return from == other.from && to == other.to;
    }
};

enum GameState {
    INPROGRESS = -1,
    WHITEWIN = 0,
    DRAW = 1,
    BLACKWIN = 2
};


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

extern std::array<Color, 2> color_arr;

extern ull a_row;
extern ull a_col;
extern std::vector<ull> row_masks;
extern std::vector<ull> col_masks;

extern uint64_t zobrist_piece_square[10][64];
extern uint64_t zobrist_side;

void initialize_zobrist();

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
