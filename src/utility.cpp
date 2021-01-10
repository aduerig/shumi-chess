#include "utility.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <cassert>

#include "globals.hpp"

namespace utility {

namespace bit {

// TODO probably will want to use __builtin_ctz once we need speed
ull lsb_and_pop(ull& board) {
    // simple linear search, slow
    ull lsb;
    for (int i = 0; i < 64; i++) {
        if ((1ULL << i) & board) {
            lsb = 1ULL << i;
            board = board & (~lsb);
            break;
        }
    }
    return lsb;
}

ull bitshift_by_color(ull board, ShumiChess::Color color, int amount) {
    if (color == ShumiChess::WHITE) {
        return board << amount;
    }
    return board >> amount;
}

} // end namespace bit

namespace representation {

ShumiChess::Color get_opposite_color(ShumiChess::Color color) {
    if (color == ShumiChess::Color::WHITE) {
        return ShumiChess::Color::BLACK;
    }
    return ShumiChess::Color::WHITE;
}

char nth_letter(int n)
{
    assert(n >= 0 && n <= 25);
    return "abcdefghijklmnopqrstuvwxyz"[n];
}

// ? whats our policy on error handling
// Likely depends on engine vs auxilary
ull acn_to_bitboard_conversion(const std::string& anc) {
    int square_number = 0;
    square_number += ('h'-anc.at(0)) + 8*(anc.at(1)-'1');
    return 1ULL << square_number;
}

std::string bitboard_to_acn_conversion(ull bitboard) {
    // TODO move this somewhere else to precompute
    std::unordered_map<ull, std::string> bitboard_to_acn_map;
    ull iterate_bitboard = 1ULL;
    for (int i = 1; i < 9; i++) {
        for (int j = 0; j < 8; j++) {
            std::string value = nth_letter(7 - j) + std::to_string(i);
            bitboard_to_acn_map[iterate_bitboard] = value;
            iterate_bitboard = iterate_bitboard << 1;
        }
    }
    return bitboard_to_acn_map[bitboard];
}


// assumes layout is:
// lower right is 2^0
// lower left is 2^7
// upper right is 2^56
// upper left is 2^63
std::string square_to_position_string(ull square) {
    std::unordered_map<int, std::string> row_to_letter = {
        {1, "a"},
        {2, "b"},
        {3, "c"},
        {4, "d"},
        {5, "e"},
        {6, "f"},
        {7, "g"},
        {8, "h"}
    };

    for (int i = 1; i <= 8; i++) {
        for (int j = 1; j <= 8; j++) {
            if (1ULL & square) {
                return row_to_letter[9 - j] + std::to_string(i);
            }
            square = square >> 1;
        }
    }
    assert(false);
}

std::string move_to_string(ShumiChess::Move move) {
    return square_to_position_string(move.from) + square_to_position_string(move.to);
}

} // end namespace representation

namespace string {

std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
    std::string::size_type curr_pos = 0;
    std::string::size_type next_del = str.find(delimiter);
    
    const std::string::size_type del_len = delimiter.length();
    std::vector<std::string> split_str;
    while (next_del != std::string::npos)
    {
        if (curr_pos != next_del) {
            split_str.emplace_back(str.begin()+curr_pos, str.begin()+next_del);
        }

        curr_pos = next_del + del_len;
        next_del = str.find(delimiter, curr_pos);
    }

    if (curr_pos != str.length()) {
        split_str.emplace_back(str, curr_pos);
    }
    
    return split_str;
}

bool starts_with(const std::string& main_str, const std::string& smaller_str) {
    return main_str.rfind(smaller_str, 0) == 0;
}

std::string join(const vector<std::string>& the_vect, const char* delim) {
   std::string new_string;
   for (std::vector<std::string>::const_iterator p = the_vect.begin(); p != the_vect.end(); p++) {
        new_string += *p;
        if (p != the_vect.end() - 1) {
        new_string += delim;
        }
   }
   return new_string;
}

} // end namespace string
} // end namespace utility