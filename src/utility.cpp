#include "utility.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

#include "globals.hpp"

namespace utility {

namespace representation {

// ? whats our policy on error handling
// Likely depends on engine vs auxilary
ull acn_to_bit_conversion(const std::string& anc) {
    int square_number = 0;
    square_number += ('h'-anc.at(0)) + 8*(anc.at(1)-'1');
    return 1ULL << square_number;
}

// black_pawns(0b00000000'11111111'00000000'00000000'00000000'00000000'00000000'00000000),
// white_pawns(0b00000000'00000000'00000000'00000000'00000000'00000000'11111111'00000000),
// black_rooks(0b10000001'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
// white_rooks(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'10000001),
// black_knights(0b01000010'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
// white_knights(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'01000010),
// black_bishops(0b00100100'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
// white_bishops(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00100100),
// black_queens(0b00010000'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
// white_queens(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00010000),
// black_king(0b00001000'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
// white_king(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00001000),


// assumes layout is:
// lower right is 1
// lower left is 8
// upper right is 56
// upper left is 63
std::string square_to_position_string(ull square) {
    std::unordered_map<ull, std::string> row_to_letter = {
        {1ULL, "a"},
        {2ULL, "b"},
        {3ULL, "c"},
        {4ULL, "d"},
        {5ULL,"e"},
        {6ULL, "f"},
        {7ULL, "g"},
        {8ULL, "h"}
    };

    for (int i = 1; i <= 8; i++) {
        for (int j = 1; j <= 8; j++) {
            std::cout << "square: " << square << ", i: " << i << ", j: " << j << std::endl;
            std::cout << "anded: " << (1ULL & square) << std::endl;
            if (1ULL & square) {
                std::cout << "done" << std::endl;
                return row_to_letter[9 - j] + std::to_string(i);
            }
            square = square >> 1;
        }
    }
    throw 1;
}

std::string move_to_string(ShumiChess::Move move) {
    return square_to_position_string(move.from) + square_to_position_string(move.to);
}

} // representation

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

} // string
} // utility