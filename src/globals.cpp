#include <globals.hpp>

namespace ShumiChess {

ull a_row = 1ULL << 0 | 1ULL << 1 | 1ULL << 2 | 1ULL << 3 |
            1ULL << 4 | 1ULL << 5 | 1ULL << 6 | 1ULL << 7;

ull a_col = 1ULL << 0 | 1ULL << 8 | 1ULL << 16 | 1ULL << 24 |
            1ULL << 32 | 1ULL << 40 | 1ULL << 48 | 1ULL << 56;

std::unordered_map<int, ull> rank_masks = {
    {1, a_row},
    {2, a_row << 8},
    {3, a_row << 16},
    {4, a_row << 24},
    {5, a_row << 32},
    {6, a_row << 40},
    {7, a_row << 48},
    {8, a_row << 56}
};
std::unordered_map<char, ull> col_masks = {
    {'h', a_col},
    {'g', a_col << 1},
    {'f', a_col << 2},
    {'e', a_col << 3},
    {'d', a_col << 4},
    {'c', a_col << 5},
    {'b', a_col << 6},
    {'a', a_col << 7}
};
} // end namespace ShumiChess