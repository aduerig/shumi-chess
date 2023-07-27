#include "utility.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <cassert>

#include "globals.hpp"

using namespace std;

namespace utility {

namespace representation {

// ? whats our policy on error handling
// Likely depends on engine vs auxilary
ull acn_to_bitboard_conversion(const string& acn) {
    int square_number = 0;
    square_number += ('h' - acn.at(0)) + 8 * (acn.at(1) - '1');
    return 1ULL << square_number;
}

char nth_letter(int n)
{
    assert(n >= 0 && n <= 25);
    return "abcdefghijklmnopqrstuvwxyz"[n];
}
string bitboard_to_acn_conversion(ull bitboard) {
    // TODO move this somewhere else to precompute
    unordered_map<ull, string> bitboard_to_acn_map;
    ull iterate_bitboard = 1ULL;
    for (int i = 1; i < 9; i++) {
        for (int j = 0; j < 8; j++) {
            string value = nth_letter(7 - j) + to_string(i);
            bitboard_to_acn_map[iterate_bitboard] = value;
            iterate_bitboard = iterate_bitboard << 1;
        }
    }
    return bitboard_to_acn_map[bitboard];
}

string move_to_string(ShumiChess::Move move) {
    return square_to_position_string(move.from) + square_to_position_string(move.to);
}

void print_bitboard(ull bitboard) {
    string builder;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            ull curr_single_bitboard = (1ULL << ((i * 8) + j));
            if (bitboard & curr_single_bitboard) {
                builder = "1" + builder;
            }
            else {
                builder = "0" + builder;
            }
        }
        if (i != 7) {
            builder = "\n" + builder;
        }
    }
    cout << builder << endl;
}

void print_gameboard(ShumiChess::GameBoard gameboard) {
    unordered_map<ull, char> bitboard_to_letter = {
        {gameboard.white_bishops, 'B'},
        {gameboard.white_knights, 'N'},
        {gameboard.white_king, 'K'},
        {gameboard.white_pawns, 'P'},
        {gameboard.white_rooks, 'R'},
        {gameboard.white_queens, 'Q'},
        {gameboard.black_bishops, 'b'},
        {gameboard.black_knights, 'n'},
        {gameboard.black_king, 'k'},
        {gameboard.black_pawns, 'p'},
        {gameboard.black_rooks, 'r'},
        {gameboard.black_queens, 'q'},
    };

    string builder(71, '-');
    for (const auto& pair : bitboard_to_letter) {
        ull bitboard = pair.first;
        char letter = pair.second;
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                ull curr_single_bitboard = (1ULL << ((i*8) + j));
                if (bitboard & curr_single_bitboard) {
                    builder[70 - ((i*9) + j)] = letter;
                }
            }
            if (i != 7) {
                builder[70 - ((i*9) + 8)] = '\n';
            }
        }
    }
    cout << builder << endl;
}

string stringify(ShumiChess::Piece piece) {
    unordered_map<ShumiChess::Piece, string> piece_strings = {
        {ShumiChess::Piece::PAWN, "pawn"}, 
        {ShumiChess::Piece::ROOK, "rook"}, 
        {ShumiChess::Piece::KNIGHT, "knight"}, 
        {ShumiChess::Piece::BISHOP, "bishop"}, 
        {ShumiChess::Piece::QUEEN, "queen"}, 
        {ShumiChess::Piece::KING, "king"} 
    };
    return piece_strings[piece];
}

// assumes layout is:
// lower right is 2^0
// lower left is 2^7
// upper right is 2^56
// upper left is 2^63
string square_to_position_string(ull square) {
    unordered_map<int, string> row_to_letter = {
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
                return row_to_letter[9 - j] + to_string(i);
            }
            square = square >> 1;
        }
    }
    return "error";
}

} // end namespace representation

namespace our_string {

vector<string> split(const string& str, const string& delimiter) {
    string::size_type curr_pos = 0;
    string::size_type next_del = str.find(delimiter);
    
    const string::size_type del_len = delimiter.length();
    vector<string> split_str;
    while (next_del != string::npos)
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

bool starts_with(const string& main_str, const string& smaller_str) {
    return main_str.rfind(smaller_str, 0) == 0;
}

string join(const vector<string>& the_vect, const char* delim) {
   string new_string;
   for (vector<string>::const_iterator p = the_vect.begin(); p != the_vect.end(); p++) {
        new_string += *p;
        if (p != the_vect.end() - 1) {
        new_string += delim;
        }
   }
   return new_string;
}

} // end namespace string
} // end namespace utility