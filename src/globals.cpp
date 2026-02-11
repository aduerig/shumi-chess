#include <globals.hpp>
#include <limits>
#include <algorithm>

#ifdef SHUMI_FORCE_ASSERTS  // Operated by the -asserts" and "-no-asserts" args to run_gui.py. By default on.
#undef NDEBUG
#endif
#include <assert.h>

using namespace std;

namespace ShumiChess {

ull a_row = 1ULL << 0 | 1ULL << 1 | 1ULL << 2 | 1ULL << 3 |
            1ULL << 4 | 1ULL << 5 | 1ULL << 6 | 1ULL << 7;

ull a_col = 1ULL << 0 | 1ULL << 8 | 1ULL << 16 | 1ULL << 24 |
            1ULL << 32 | 1ULL << 40 | 1ULL << 48 | 1ULL << 56;

vector<ull> row_masks = {
    a_row,
    a_row << 8,
    a_row << 16,
    a_row << 24,
    a_row << 32,
    a_row << 40,
    a_row << 48,
    a_row << 56
};

vector<ull> col_masks = {
    a_col << 7,
    a_col << 6,
    a_col << 5,
    a_col << 4,
    a_col << 3,
    a_col << 2,
    a_col << 1,
    a_col
};


class MyPRNG {
private:
    // Parameters for a 64-bit Linear Congruential Generator (LCG) [Numerical Recipes (3rd edition)]
    static constexpr uint64_t a = 6364136223846793005ULL; // multiplier
    static constexpr uint64_t c = 1442695040888963407ULL; // increment
    static constexpr uint64_t m = std::numeric_limits<uint64_t>::max(); // modulus
    uint64_t current_state {123456789};

public:
    MyPRNG() {}

    uint64_t get_random_number() {
        current_state = (a * current_state + c) % m; // LCG formula
        return current_state;
    }
};


// !TODO: Make this the same as array probably. Single dimensional array? This is unprotected!
// needs an assert at least!
uint64_t zobrist_piece_square[12][64];
uint64_t zobrist_enpassant[8];
uint64_t zobrist_castling[16];
uint64_t zobrist_side;



// One number for each piece at each square
// One number to indicate the side to move is black
// Four numbers to indicate the castling rights, though usually 16 (2^4) are used for speed
// Eight numbers to indicate the file of a valid En passant square, if any
// This leaves us with an array with 793 (12*64 + 1 + 16 + 8)
void initialize_zobrist() {
    MyPRNG randomer;
    
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 64; j++) {
            zobrist_piece_square[i][j] = randomer.get_random_number();
        }
    }
    for (int i = 0; i < 8; i++) {
        zobrist_enpassant[i] = randomer.get_random_number();
    }
    for (int i = 0; i < 16; i++) {
        zobrist_castling[i] = randomer.get_random_number();
    }
    zobrist_side = randomer.get_random_number();
}

Move MoveSet(Color c, Piece p, ull frm, ull to)
{
    Move m;
    m.color = c;
    m.piece_type = p;
    m.from = frm;
    m.to = to;
    return m;
}
Move MoveSet2(Color c, Piece p, ull frm, ull to, Piece a)
{
    Move m;
    m.color = c;
    m.piece_type = p;
    m.from = frm;
    m.to = to;
    m.capture = a;
    return m;
}
Move MoveSet3(Color c, Piece p, ull frm, ull to, Piece a, Piece b)
{
    Move m;
    m.color = c;
    m.piece_type = p;
    m.from = frm;
    m.to = to;
    m.capture = a;
    m.promotion = b;
    return m;
}

//
// Precomputes 8 directional "ray" bitboards for every board square (0..63).
//
// A "ray" is a 64-bit mask with 1-bits on every square reachable by sliding
// from the origin square in a given direction until the edge of the board
// (origin square itself is NOT included).
//
// These tables support fast rook/bishop/queen move generation:
//   - masked_blockers = all_pieces & ray[square]   (pieces that block that ray)
//   - if blockers exist, trim the ray past the nearest blocker
//   - if no blockers exist, the attack set is the full ray
//
// square_to_x[s] and square_to_y[s] store file/rank coordinates derived from
// the engine’s square indexing (here: x = s % 8, y = s / 8).
//
// Sentinel index 64:
//   ray[64] is set to 0 for every direction. This allows code to use 64 as a
//   "no blocker" sentinel (e.g., blockerSquare = 64) without out-of-bounds
//   access: ~ray[64] & ray[square] becomes (~0) & ray[square] == ray[square].

array<ull, 64> square_to_y = {};
array<ull, 64> square_to_x = {};

array<ull, 65> north_east_square_ray = {};
array<ull, 65> north_west_square_ray = {};
array<ull, 65> south_east_square_ray = {};
array<ull, 65> south_west_square_ray = {};

array<ull, 65> north_square_ray = {};
array<ull, 65> south_square_ray = {};
array<ull, 65> east_square_ray = {};
array<ull, 65> west_square_ray = {};

void initialize_rays() {
    for (int square = 0; square < 64; square++) {
        square_to_y[square] = (int) square / 8;
        square_to_x[square] = (int) square % 8;
    }

    for (int square = 0; square < 64; square++) {
        int square_x = square_to_x[square];
        int square_y = square_to_y[square];
        for (int i = 1; i < 8; i++) {
            if (square_x - i >= 0 && square_y + i < 8) {
                north_east_square_ray[square] |= 1ULL << (square + 8*i - i);
            }
            if (square_x + i < 8 && square_y + i < 8) {
                north_west_square_ray[square] |= 1ULL << (square + 8*i + i);
            }
            if (square_x - i >= 0 && square_y - i >= 0) {
                south_east_square_ray[square] |= 1ULL << (square - 8*i - i);
            }
            if (square_x + i < 8 && square_y - i >= 0) {
                south_west_square_ray[square] |= 1ULL << (square - 8*i + i);
            }

            if (square_y + i < 8) {
                north_square_ray[square] |= 1ULL << (square + 8*i);
            }
            if (square_y - i >= 0) {
                south_square_ray[square] |= 1ULL << (square - 8*i);
            }
            if (square_x - i >= 0) {
                east_square_ray[square] |= 1ULL << (square - i);
            }
            if (square_x + i < 8) {
                west_square_ray[square] |= 1ULL << (square + i);
            }
        }
    }
    // Sentinals for the "64" case, when 
    north_east_square_ray[64] = 0ULL;
    north_west_square_ray[64] = 0ULL;
    south_east_square_ray[64] = 0ULL;
    south_west_square_ray[64] = 0ULL;
    north_square_ray[64] = 0ULL;
    south_square_ray[64] = 0ULL;
    east_square_ray[64] = 0ULL;
    west_square_ray[64] = 0ULL;
}

vector<Piece> promotion_values = {
    Piece::BISHOP,
    Piece::KNIGHT,
    Piece::ROOK,
    Piece::QUEEN
};


const char* str_from_GamePhase(int phse) {
    switch (phse)
    {
        case OPENING:
            return "OPENING";
        case MIDDLE_EARLY:
            return "MIDDLE_EARLY";
        case MIDDLE:
            return "MIDDLE";
        case ENDGAME:
            return "ENDGAME";
        case ENDGAME_LATE:
            return "ENDGAME_LATE";
        default:
            return "????";
    }
};


bool is_move_in_list(const Move& mov, const std::vector<Move>& mvs)
{
    return std::find(mvs.begin(), mvs.end(), mov) != mvs.end();
    // for (Move m : mvs) {
    //     if  (m == mov) {
    //         return true;
    //     }
    // }
    // return false;
}


} // end namespace ShumiChess