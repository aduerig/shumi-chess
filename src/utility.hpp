#pragma once

//TODO shares a name with builtin lib
#include <algorithm> 
#include <cctype>
#include <iterator>
#include <locale>
#include <string>
#include <vector>
#include <unordered_map>


#include "globals.hpp"
#include "gameboard.hpp"

using namespace std;

namespace utility {

namespace bit {

ull bitshift_by_color(ull, ShumiChess::Color, int);
ull lsb_and_pop(ull&);
int bitboard_to_square(ull);

} // end namespace bit

namespace representation {

ull acn_to_bitboard_conversion(const std::string&);
std::string bitboard_to_acn_conversion(ull);
inline const ShumiChess::Color get_opposite_color(const ShumiChess::Color color) {
    return (ShumiChess::Color) (1 - (int) color);
}
std::string move_to_string(ShumiChess::Move);
void print_bitboard(ull);
void print_gameboard(ShumiChess::GameBoard);
std::string stringify(ShumiChess::Piece);
std::string square_to_position_string(ull);

} // end namespace representation

// ? should we really clash in namespace name here
namespace string {

// Split an input string according to some string delimiter.
// Uses space by default. 
std::vector<std::string> split(const std::string&, const std::string& = " ");
std::string join(const vector<std::string>&, const char*);
bool starts_with(const std::string& main_str, const std::string& smaller_str);

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

} // end namespace string
} // end namespace utility