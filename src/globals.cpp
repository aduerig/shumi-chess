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

// both start at bottom
// e.g.
// 10000000
// 01000000
// 00100000
// 00010000
// 00001000
// 00000100
// 00000010
// 00000001
std::unordered_map<int, ull> down_right_diagonals = {
    {1, 1ULL << 7},
    {2, 1ULL << 6 | 1ULL << 15},
    {3, 1ULL << 5 | 1ULL << 14 | 1ULL << 23},
    {4, 1ULL << 4 | 1ULL << 13 | 1ULL << 22 | 1ULL << 31},
    {5, 1ULL << 3 | 1ULL << 12 | 1ULL << 21 | 1ULL << 30 | 1ULL << 39},
    {6, 1ULL << 2 | 1ULL << 11 | 1ULL << 20 | 1ULL << 29 | 1ULL << 38 | 1ULL << 47},
    {7, 1ULL << 1 | 1ULL << 10 | 1ULL << 19 | 1ULL << 28 | 1ULL << 37 | 1ULL << 46 | 1ULL << 55},
    {8, 1ULL | 1ULL << 9 | 1ULL << 18 | 1ULL << 27 | 1ULL << 36 | 1ULL << 45 | 1ULL << 54 | 1ULL << 63},
    {9, 1ULL << 62 | 1ULL << 53 | 1ULL << 44 | 1ULL << 35 | 1ULL << 26 | 1ULL << 17| 1ULL << 8},
    {10, 1ULL << 61 | 1ULL << 52 | 1ULL << 43 | 1ULL << 34 | 1ULL << 25 | 1ULL << 16},
    {11, 1ULL << 60 | 1ULL << 51 | 1ULL << 42 | 1ULL << 33 | 1ULL << 24},
    {12, 1ULL << 59 | 1ULL << 50 | 1ULL << 41 | 1ULL << 32},
    {13, 1ULL << 58 | 1ULL << 49 | 1ULL << 40},
    {14, 1ULL << 57 | 1ULL << 48},
    {15, 1ULL << 56}
};

// eg.
// 00000001
// 00000010
// 00000100
// 00001000
// 00010000
// 00100000
// 01000000
// 10000000
std::unordered_map<int, ull> down_left_diagonals = {
    {1, 1ULL},
    {2, 1ULL << 1 | 1ULL << 8},
    {3, 1ULL << 2 | 1ULL << 9 | 1ULL << 16},
    {4, 1ULL << 3 | 1ULL << 10 | 1ULL << 17 | 1ULL << 24},
    {5, 1ULL << 4 | 1ULL << 11 | 1ULL << 18 | 1ULL << 25 | 1ULL << 32},
    {6, 1ULL << 5 | 1ULL << 12 | 1ULL << 19 | 1ULL << 26 | 1ULL << 33 | 1ULL << 40},
    {7, 1ULL << 6 | 1ULL << 13 | 1ULL << 20 | 1ULL << 27 | 1ULL << 34 | 1ULL << 41 | 1ULL << 48},
    {8, 1ULL << 7 | 1ULL << 14 | 1ULL << 21 | 1ULL << 28 | 1ULL << 35 | 1ULL << 42 | 1ULL << 49 | 1ULL << 56},
    {9, 1ULL << 57 | 1ULL << 50 | 1ULL << 43 | 1ULL << 36 | 1ULL << 29 | 1ULL << 22 | 1ULL << 15},
    {10, 1ULL << 58 | 1ULL << 51 | 1ULL << 44 | 1ULL << 37 | 1ULL << 30 | 1ULL << 23},
    {11, 1ULL << 59 | 1ULL << 52 | 1ULL << 45 | 1ULL << 38 | 1ULL << 31},
    {12, 1ULL << 60 | 1ULL << 53 | 1ULL << 46 | 1ULL << 39},
    {13, 1ULL << 61 | 1ULL << 54 | 1ULL << 47},
    {14, 1ULL << 62 | 1ULL << 55},
    {15, 1ULL << 63}
};


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