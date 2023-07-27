#include <globals.hpp>

namespace ShumiChess {

ull a_row = 1ULL << 0 | 1ULL << 1 | 1ULL << 2 | 1ULL << 3 |
            1ULL << 4 | 1ULL << 5 | 1ULL << 6 | 1ULL << 7;

ull a_col = 1ULL << 0 | 1ULL << 8 | 1ULL << 16 | 1ULL << 24 |
            1ULL << 32 | 1ULL << 40 | 1ULL << 48 | 1ULL << 56;

std::vector<ull> row_masks = {
    a_row,
    a_row << 8,
    a_row << 16,
    a_row << 24,
    a_row << 32,
    a_row << 40,
    a_row << 48,
    a_row << 56
};

std::vector<ull> col_masks = {
    a_col << 7,
    a_col << 6,
    a_col << 5,
    a_col << 4,
    a_col << 3,
    a_col << 2,
    a_col << 1,
    a_col
};


// !TODO: Make this the same as std::array probably
int zobrist_piece_square[12][64];
int zobrist_enpassant[8];
int zobrist_castling[16];
int zobrist_side;

// One number for each piece at each square
// One number to indicate the side to move is black
// Four numbers to indicate the castling rights, though usually 16 (2^4) are used for speed
// Eight numbers to indicate the file of a valid En passant square, if any
// This leaves us with an array with 793 (12*64 + 1 + 16 + 8)
void initialize_zobrist() {
    srand(69);
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 64; j++) {
            zobrist_piece_square[i][j] = std::rand();
        }
    }
    for (int i = 0; i < 8; i++) {
        zobrist_enpassant[i] = std::rand();
    }
    for (int i = 0; i < 16; i++) {
        zobrist_castling[i] = std::rand();
    }
    zobrist_side = std::rand();
}


std::array<ull, 64> square_to_y = {};
std::array<ull, 64> square_to_x = {};

std::array<ull, 65> north_east_square_ray = {};
std::array<ull, 65> north_west_square_ray = {};
std::array<ull, 65> south_east_square_ray = {};
std::array<ull, 65> south_west_square_ray = {};

std::array<ull, 65> north_square_ray = {};
std::array<ull, 65> south_square_ray = {};
std::array<ull, 65> east_square_ray = {};
std::array<ull, 65> west_square_ray = {};

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
    north_east_square_ray[64] = 0ULL;
    north_west_square_ray[64] = 0ULL;
    south_east_square_ray[64] = 0ULL;
    south_west_square_ray[64] = 0ULL;
    north_square_ray[64] = 0ULL;
    south_square_ray[64] = 0ULL;
    east_square_ray[64] = 0ULL;
    west_square_ray[64] = 0ULL;
}

std::vector<Piece> promotion_values = {
    Piece::BISHOP,
    Piece::KNIGHT,
    Piece::QUEEN,
    Piece::ROOK
};

} // end namespace ShumiChess