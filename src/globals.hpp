#pragma once

#include <cinttypes>
#include <optional>
#include <unordered_map>
#include <vector>
#include <ostream>
#include <iostream>
#include <array>
#include <cstdlib>
// #include <random>

#undef NDEBUG
#define NDEBUG         // Define (uncomment) this to disable asserts
#include <assert.h>


typedef unsigned long long ull;

namespace ShumiChess {
enum Color {
//enum Color : std::uint8_t {
    WHITE = 0,
    BLACK,
};

enum Piece {     // Pieces must be in this order!
    PAWN = 0,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING,
    NONE,
};

// TODO think about if this is the right way to represent a move
// NOTE: Can this be a class? How would it help?
// 56 bytes.
struct Move {
    Color color = ShumiChess::WHITE;
    Piece piece_type =  Piece::NONE;       // As in "pawn", "queen", etc. that is moving.
    ull from = 0ULL;   // bitboard (but with only one bit set)
    ull to = 0ULL;     // bitboard (but with only one bit set)
    Piece capture = Piece::NONE;
    Piece promotion = Piece::NONE;
    uint8_t black_castle_rights = 0b00000011;
    uint8_t white_castle_rights = 0b00000011;
    ull en_passant_rights = 0;              // Note this is a bitboard?
    bool is_en_passent_capture = false;    // bools are hopefully 1 byte
    bool is_castle_move = false;

    // This means two Moves are considered equal if they go from the same square to the same square.
    // NOTE: Is this right ? It only checks the squares, not the pieces on the square.
    // But wait, the piece on the square, and the rest of the board already encoded in the FEN 
    // which is the "outer map" of the hashTable? I have no idea what im talking about. In any case,
    // the promotion piece must be added to the equality. Not sure about the "en_passant_rights"?

    bool operator==(const Move &other) const {
        //return ((from == other.from) && (to == other.to));
        return ((from == other.from) && (to == other.to) && (promotion == other.promotion));
    }
    
};

// ? is this best way to number
// NOTE: cant we just renumber it from 0 ? NO, somehow the python requires this.
//   Wait I think its just the line "game_over_cache() == -1:" in show_board.py.
// ? Should we try to make most checked things = 0 for magical compiler is zero optimizations? 
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

extern uint64_t zobrist_piece_square[12][64];
extern uint64_t zobrist_enpassant[8];           // not used yet
extern uint64_t zobrist_castling[16];           // not used yet
extern uint64_t zobrist_side;

void initialize_zobrist();
inline uint64_t zobrist_piece_square_get(int i, int j) {
    assert (i>= 0);
    assert (i< 12);
    assert (j>= 0);
    assert (j< 64);
    return zobrist_piece_square[i][j];
}

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
