#pragma once

#include <cinttypes>
#include <optional>
#include <unordered_map>
#include <vector>
#include <ostream>
#include <iostream>
#include <array>
#include <cstdlib>
#include <cstddef>

#include "score.hpp"

#ifdef SHUMI_FORCE_ASSERTS  // Operated by the -asserts" and "-no-asserts" args to run_gui.py. By default on.
#undef NDEBUG
#endif
#include <assert.h>


typedef unsigned long long ull;
typedef uint8_t Square;

// Forward declare what globals.hpp needs, without including utility.hpp (avoids cycles)
namespace utility
{
    namespace bit
    {
        int bitboard_to_lowest_square_fast(ull bitboard);
        int bitboard_to_highest_square_fast(ull bitboard);
    }
}

namespace tables::movegen
{
    struct StraightMagicEntry {
        ull mask = 0ULL;
        ull magic = 0ULL;
        unsigned int shift = 0;
        std::size_t offset = 0;
    };

    constexpr std::size_t straight_magic_attack_table_size = 102400;

    extern std::array<StraightMagicEntry, 64> straight_magic_entries;
    extern std::array<ull, straight_magic_attack_table_size> straight_magic_attack_table;

    struct DiagonalMagicEntry {
        ull mask = 0ULL;
        ull magic = 0ULL;
        unsigned int shift = 0;
        std::size_t offset = 0;
    };

    constexpr std::size_t diagonal_magic_attack_table_size = 5248;

    extern std::array<DiagonalMagicEntry, 64> diagonal_magic_entries;
    extern std::array<ull, diagonal_magic_attack_table_size> diagonal_magic_attack_table;

    ull get_straight_magic_attack(ull all_pieces_but_self, int square);
    ull get_diagonal_magic_attack(ull all_pieces_but_self, int square);
}



namespace ShumiChess {

enum Color : std::uint8_t {
    WHITE = 0,
    BLACK,
};

constexpr int NUM_PIECES = 7; 
enum Piece : std::uint8_t {     // These MUST be in this order
    PAWN = 0,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING,
    NONE,
};

constexpr Square NO_SQUARE = 64;

//                                          12345678
//  These constants MUST be these values
constexpr int FLAGS_CASTLE_NONE         = 0b00000000;
constexpr int FLAGS_CASTLE_EITHER       = 0b00000011;
constexpr int FLAGS_CASTLE_KING         = 0b00000001;
constexpr int FLAGS_CASTLE_QUEEN        = 0b00000010;
constexpr int FLAGS_IS_EP_CAPTURE       = 0b00010000;
constexpr int FLAGS_IS_CASTLE_MOVE      = 0b00100000;

constexpr int FLAGS_CASTLE_ALL_BITS     = 0b00001111;


// NOTE: Can this be a class? How would it help?
struct Move {
  
    Square fromSQ = NO_SQUARE;
    Square toSQ = NO_SQUARE;

    Square en_passant_landingSQ = NO_SQUARE;  // A 1-bitboard, the square where the capturing pawn would land in an en-passant capture

    Color color = ShumiChess::WHITE;
    Piece piece_type =  Piece::NONE;       // As in "pawn", "queen", etc. that is moving.

    Piece capture = Piece::NONE;
    Piece promotion = Piece::NONE;

    // Move flags  (FLAGS_ constants)
    //      bit 0,1 :   White castling rights   (only used by pushMove, to xfer into the gameboard)
    //      bit 2,3 :   Black castling rights   (only used by pushMove, to xfer into the gameboard)
    //      bit 4   :   is a enpassant capture  (off by default)
    //      bit 5   :   is a castle move        (off by default)
    uint8_t flags = (FLAGS_CASTLE_EITHER << 2) | FLAGS_CASTLE_EITHER;

    Move() = default;   // ← put it here

   // --- NEW FULL FIELD CONSTRUCTOR (used for speed, in add_psuedo_move_to_vector()) ---
    Move(
         Square fromSQ_,
         Square toSQ_,
         Square en_passant_landing_,
         Color color_,
         Piece piece_type_,
         Piece capture_,
         Piece promotion_,
         uint8_t flags_)
        :
          fromSQ(fromSQ_),
          toSQ(toSQ_),
          en_passant_landingSQ(en_passant_landing_),
          color(color_),
          piece_type(piece_type_),
          capture(capture_),
          promotion(promotion_),
          flags(flags_)
    {
    }


    // This means two Moves are considered equal if they go from the same square to the same square.
    // NOTE: Is this right ? It only checks the squares, not the pieces on the square.
    // But wait, the piece on the square, and the rest of the board already encoded in the FEN 
    // which is the "outer map" of the hashTable? I have no idea what im talking about. In any case,
    // the promotion piece must be added to the equality. But should we (or do we need to) compare 
    // all the structure elements? I see that these three items is enough for ACN+promo (UCI) notation.
    bool operator==(const Move &other) const {
        // bool bA = ((from == other.from) && (to == other.to) && (promotion == other.promotion));
        // bool bB = ((fromSQ == other.fromSQ) && (toSQ == other.toSQ) && (promotion == other.promotion));
        // assert (bA == bB);
        return ((fromSQ == other.fromSQ) && (toSQ == other.toSQ) && (promotion == other.promotion));
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
enum GamePhase {        // We must stay in this order.
    OPENING = 0,
    MIDDLE_EARLY,
    MIDDLE,         // both sides castled by now
    ENDGAME,
    ENDGAME_LATE
};
const char* str_from_GamePhase(int phse);
   

enum EvalPersons {      // These are all "ai" no human here.
    RANDOM = 0,
    SLUG,
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

extern uint64_t zobrist_piece_square[12][64];
extern uint64_t zobrist_enpassant[8];
extern uint64_t zobrist_castling[16];
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



inline ull get_diagonal_attacks(ull all_pieces_but_self, int square)
{
    assert(0);
    ull ne_attacks;
    ull nw_attacks;
    ull se_attacks;
    ull sw_attacks;
    Square blocked_square;

    // up right
    ull masked_blockers_ne = all_pieces_but_self & north_east_square_ray[square];
    if (masked_blockers_ne != 0ULL) {
        blocked_square = utility::bit::bitboard_to_lowest_square_fast(masked_blockers_ne);
        ne_attacks = ~north_east_square_ray[blocked_square] & north_east_square_ray[square];
    } else {
        ne_attacks = north_east_square_ray[square];
    }

    // up left
    ull masked_blockers_nw = all_pieces_but_self & north_west_square_ray[square];
    if (masked_blockers_nw != 0ULL) {
        blocked_square = utility::bit::bitboard_to_lowest_square_fast(masked_blockers_nw);
        nw_attacks = ~north_west_square_ray[blocked_square] & north_west_square_ray[square];
    } else {
        nw_attacks = north_west_square_ray[square];
    }

    // down right
    ull masked_blockers_se = all_pieces_but_self & south_east_square_ray[square];
    if (masked_blockers_se != 0ULL) {
        blocked_square = utility::bit::bitboard_to_highest_square_fast(masked_blockers_se);
        se_attacks = ~south_east_square_ray[blocked_square] & south_east_square_ray[square];
    } else {
        se_attacks = south_east_square_ray[square];
    }

    // down left
    ull masked_blockers_sw = all_pieces_but_self & south_west_square_ray[square];
    if (masked_blockers_sw != 0ULL) {
        blocked_square = utility::bit::bitboard_to_highest_square_fast(masked_blockers_sw);
        sw_attacks = ~south_west_square_ray[blocked_square] & south_west_square_ray[square];
    } else {
        sw_attacks = south_west_square_ray[square];
    }

    return (ne_attacks | nw_attacks | se_attacks | sw_attacks);
}


inline ull get_diagonal_attacks_mbb(ull all_pieces_but_self, int square)
{
    // Get the precomputed magic-bitboard data for this bishop/diagonal square.
    // This entry contains:
    //   mask   : the relevant blocker squares for this square, excluding edge squares
    //   magic  : the magic multiplier chosen for this square
    //   shift  : how far to shift after multiplication to form a compact index
    //   offset : where this square's attack table begins in the shared table
    const tables::movegen::DiagonalMagicEntry& entry =
        tables::movegen::diagonal_magic_entries[square];

    // Keep only the occupied squares that can actually affect diagonal attacks
    // from this square. Other pieces on the board are irrelevant.
    const ull blockers = all_pieces_but_self & entry.mask;

    // Convert this blocker pattern into a small table index.
    // The magic multiplication scrambles the relevant blocker bits so that,
    // after shifting, each possible blocker pattern maps to the correct
    // precomputed attack entry.
    const std::size_t magic_index = (blockers * entry.magic) >> entry.shift;

    // Look up the bishop/diagonal attacks for this square and blocker pattern.
    // The offset selects this square's section of the shared attack table.
    return tables::movegen::diagonal_magic_attack_table[entry.offset + magic_index];
}

// codex resume 019e8629-54a2-7e93-a367-24a16b94b167
//__declspec(noinline) 
inline ull get_straight_attacks(ull all_pieces_but_self, int square)
{
    assert(0);
    ull n_attacks;
    ull s_attacks;
    ull e_attacks;
    ull w_attacks;
    Square blocked_square;

    // north
    ull masked_blockers_n = all_pieces_but_self & north_square_ray[square];
    if (masked_blockers_n) {
        blocked_square = utility::bit::bitboard_to_lowest_square_fast(masked_blockers_n);
        n_attacks = ~north_square_ray[blocked_square] & north_square_ray[square];
    } else {
        n_attacks = north_square_ray[square];
    }

    // south
    ull masked_blockers_s = all_pieces_but_self & south_square_ray[square];
    if (masked_blockers_s) {
        blocked_square = utility::bit::bitboard_to_highest_square_fast(masked_blockers_s);
        s_attacks = ~south_square_ray[blocked_square] & south_square_ray[square];
    } else {
        s_attacks = south_square_ray[square];
    }

    // left
    ull masked_blockers_w = all_pieces_but_self & west_square_ray[square];
    if (masked_blockers_w) {
        blocked_square = utility::bit::bitboard_to_lowest_square_fast(masked_blockers_w);
        w_attacks = ~west_square_ray[blocked_square] & west_square_ray[square];
    } else {
        w_attacks = west_square_ray[square];
    }

    // right
    ull masked_blockers_e = all_pieces_but_self & east_square_ray[square];
    if (masked_blockers_e) {
        blocked_square = utility::bit::bitboard_to_highest_square_fast(masked_blockers_e);
        e_attacks = ~east_square_ray[blocked_square] & east_square_ray[square];
    } else {
        e_attacks = east_square_ray[square];
    }

    return (n_attacks | s_attacks | w_attacks | e_attacks);
}

// __declspec(noinline)
inline ull get_straight_attacks_mbb(ull all_pieces_but_self, int square)
{
    const tables::movegen::StraightMagicEntry& entry = tables::movegen::straight_magic_entries[square];
    const ull blockers = all_pieces_but_self & entry.mask;
    const std::size_t magic_index = (blockers * entry.magic) >> entry.shift;
    return tables::movegen::straight_magic_attack_table[entry.offset + magic_index];
}



} // end namespace ShumiChess
