#pragma once

#include <cinttypes>
#include <optional>
#include <unordered_map>
#include <vector>
#include <ostream>
#include <iostream>
#include <array>
#include <cstdlib>

#include "globals.hpp"

#ifdef SHUMI_FORCE_ASSERTS  // Operated by the -asserts" and "-no-asserts" args to run_gui.py. By default on.
#undef NDEBUG
#endif
#include <assert.h>


#define SHORT_SIZES     // minimize size of the Move structure.

typedef unsigned long long ull;

namespace ShumiChess {

#ifdef SHORT_SIZES
enum Color : std::uint8_t {
#else 
enum Color {
#endif
    WHITE = 0,
    BLACK,
};

#ifdef SHORT_SIZES
enum Piece : std::uint8_t {
#else
enum Piece {     // Pieces must be in this order!
#endif
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
// Now its at 32 Whopee.
struct Move {
  
    ull from = 0ULL;   // bitboard (but with only one bit set)
    ull to = 0ULL;     // bitboard (but with only one bit set)
    ull en_passant_rights = 0;              // Note this is a bitboard?

    Color color = ShumiChess::WHITE;
    Piece piece_type =  Piece::NONE;       // As in "pawn", "queen", etc. that is moving.

    Piece capture = Piece::NONE;
    Piece promotion = Piece::NONE;
    uint8_t black_castle_rights = 0b00000011;
    uint8_t white_castle_rights = 0b00000011;

    bool is_en_passent_capture = false;    // bools are hopefully 1 byte
    bool is_castle_move = false;

    // This means two Moves are considered equal if they go from the same square to the same square.
    // NOTE: Is this right ? It only checks the squares, not the pieces on the square.
    // But wait, the piece on the square, and the rest of the board already encoded in the FEN 
    // which is the "outer map" of the hashTable? I have no idea what im talking about. In any case,
    // the promotion piece must be added to the equality. Not sure about the "en_passant_rights"?

    bool operator==(const Move &other) const {
        return ((from == other.from) && (to == other.to) && (promotion == other.promotion));
    }
    
};

Move MoveSet(Color c, Piece p, ull frm, ull to);
Move MoveSet2(Color c, Piece p, ull frm, ull to, Piece a);
Move MoveSet3(Color c, Piece p, ull frm, ull to, Piece a, Piece b);


enum GameState {
    INPROGRESS = 0,
    WHITEWIN,
    DRAW,
    BLACKWIN
};
//
//  GamePhase::OPENING:
//      1. Castle.
//      2. Dont occupy center with queen. 
//      3. Prevent stupid-bishop pattern. 
//      4. Dont attack enemy king squares yet. (do it in all phases afterward)
//
//  GamePhase::MIDDLE_EARLY:
//      1. Castle (half strength motivator)
//
//  GamePhase::MIDDLE:
//
//  GamePhase::ENDGAME
// 
//  GamePhase::ENDGAME_LATE
//
//
enum GamePhase {
    OPENING = 0,
    MIDDLE_EARLY,
    MIDDLE,         // both sides castled by now
    ENDGAME,
    ENDGAME_LATE
};
const char* str_from_GamePhase(int phse);
   

enum EvalPersons {
    MATERIAL_ONLY = 0,
    CRAZY_IVAN,
    UNCLE_SHUMI
};

bool is_move_in_list(const Move& mov, const std::vector<Move>& mvs);


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

// enum Col {
//     COL_A = 0,
//     COL_B = 1,
//     COL_C = 2,
//     COL_D = 3,
//     COL_E = 4,
//     COL_F = 5,
//     COL_G = 6,
//     COL_H = 7
// };

enum ColHA {
    COL_H = 0,
    COL_G = 1,
    COL_F = 2,
    COL_E = 3,
    COL_D = 4,
    COL_C = 5,
    COL_B = 6,
    COL_A = 7
};


// TODO move all this to movegen

extern ull a_row;
extern ull a_col;
extern std::vector<ull> row_masks;
extern std::vector<ull> col_masks;
extern std::vector<ull> col_masksHA;

extern uint64_t zobrist_piece_square[12][64];
extern uint64_t zobrist_enpassant[8];           // not used yet
extern uint64_t zobrist_castling[16];           // not used yet
extern uint64_t zobrist_side;

void initialize_zobrist();
inline uint64_t zobrist_piece_square_get(int i, int j) {
    // assert (i>= 0);
    // assert (i< 12);
    // assert (j>= 0);
    // assert (j< 64);
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
